/*
 *  程序名: rinetd.cpp, 网络代理(反向)服务程序-外网端
 *  作者: ZEL
 *
 *  ----------------------------------------------------------------
 *  | 参数文件模版:                                                        
 *  |    # 监听端口 目标IP 目标端口                                       
 *  |    5005 124.70.51.197 5005    # demo02程序的通讯端口, 用于测试       
 *  |    5006 124.70.51.197 3306    # MySQL数据库的端口                  
 *  |    5021 124.70.51.197 1521    # Oracle数据库的端口                 
 *  |    5022 124.70.51.197 22      # ssh协议的端口                      
 *  |    5080 124.70.51.197 8080    # 数据服务总线的端口  
 *  |              
 *  | 存放目录: 
 *  |    /etc/rinetd.conf                                                              
 *  ----------------------------------------------------------------
 */

#include "_public.h"

#define  MAXSOCK  1024

CLogFile  logfile;       // 日志文件操作类
CPActive  PActive;       // 进程心跳操作类


// 代理路由参数的结构体
struct st_route {
    int  listenport;           // 本地监听的通信端口
    int  listensock;           // 本地监听的socket
    int  dstport;              // 目标主机的端口
    char dstip[31];            // 目标主机的IP地址
};


vector<struct st_route> vroute;       // 代理路由参数的容器    


int  clientsocks[MAXSOCK];            // 存放每个连接对端的socket的值
int  clientatime[MAXSOCK];            // 存放没给连接最后一次收发报文的时间
int  cmdlistensock = 0;               // 服务端监听内网客户端的socket
int  cmdconnsock = 0;                 // 内网客户端与服务端的控制通道
int  epollfd = 0;                     // epoll句柄
int  tfd = 0;                         // 定时器句柄



// 程序帮助文档
void _help();

// 把代理路由参数加载到vroute容器
bool loadRoute(const char *inifile);

// 网络代理程序主函数
bool _rinetd();

// 初始化服务端的监听端口
int initServer(int port, bool isblock = false);

// 接收客户端的新连接
int acceptClient(const int listenfd, bool isblock = false);

// 程序退出和信号2,15的处理函数
void EXIT(int sig);




int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 4) {
        _help();
        return -1;
    }

    // 关闭全部信号和输入输出
    // 设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
    // 但请不要用 "kill -9 + 进程号" 强行终止
    CloseIOAndSignal();
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[1]);
        return -1;
    }

    // 把进程的心跳信息写入共享内存
    PActive.AddPInfo(30, "rinetd"); 

    // 加载代理路由参数配置文件
    if (!loadRoute(argv[2])) {
        logfile.Write("loadRoute(%s) failed.\n", argv[2]);
        return -1;
    }
    logfile.Write("load inifile(%s) ok.\n", argv[2]);

    // 初始化监听内网程序的端口, 等待内网发起连接, 建立控制通道
    if ((cmdlistensock = initServer(atoi(argv[3]))) < 0) {
        logfile.Write("initServer(%d) failed.\n", atoi(argv[3]));
        EXIT(-1);
    }

    // 等待内网程序的连接请求, 建立控制通道
    if ((cmdconnsock = acceptClient(cmdlistensock)) < 0) {
        logfile.Write("acceptClient() failed.\n");
        EXIT(-1);
    }
    logfile.Write("与内部的控制通道已建立(cmdconnsock = %d)\n", cmdconnsock);

    // 网络代理程序主函数
    if (!_rinetd()) {
        logfile.Write("_rinetd() failed.\n");
        EXIT(-1);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using : ./rinetd.out logfile inifile cmdport\n");
    printf("Example : ./rinetd.out /tmp/rinetd.log /etc/rinetd.conf 5000\n");
    printf("          /project/tools/bin/procctl 5 /project/tools/bin/inetd /tmp/inetd.log /etc/rinetd.conf 5000\n\n");  
    printf("logfile 本程序运行的日志文件名\n");
    printf("inifile 代理服务参数配置文件\n");
    printf("cmdport 与内网代理程序的通讯端口\n\n");
}


bool _rinetd() {

    // 初始化服务端的监听socket
    for (int i = 0; i < vroute.size(); i++) {
        vroute[i].listensock = initServer(vroute[i].listenport, true);
        if (vroute[i].listensock < 0) {
            logfile.Write("initServer(%d) failed.\n", vroute[i].listenport);
            EXIT(-1);
        } 
    } 

    // 创建epoll句柄
    epollfd = epoll_create(1);
   
    struct epoll_event ev;       // 声明事件的数据结构
    for (int i = 0; i < vroute.size(); i++) {
        // 为监听的socket准备可读事件 
        ev.data.fd = vroute[i].listensock;       // 指定事件的自定义数据, 会随着epoll_wait()函数返回的事件一并返回
        ev.events = EPOLLIN;                     // 关心读事件

        // 把监听的socket的事件加入的epollfd中
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, vroute[i].listensock, &ev) == -1) {
            logfile.Write("epoll_ctl() failed.\n");
            return false;
        }

    }

    // 创建定时器句柄
    tfd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC | TFD_CLOEXEC);

    // 设置定时器
    struct itimerspec timeout; memset(&timeout, 0, sizeof(timeout));
    timeout.it_value.tv_sec = 20; timeout.it_value.tv_nsec = 0;
    timerfd_settime(tfd, 0, &timeout, NULL);

    // 为定时器准备时间
    ev.data.fd = tfd;
    ev.events = EPOLLIN | EPOLLET;      // 读事件, 注意! 一定要设置为边缘触发
    epoll_ctl(epollfd, EPOLL_CTL_ADD, tfd, &ev);

    struct epoll_event evs[10];       // 声明epoll_wait()函数返回的事件的数据结构

    while (true) {

        int infds = epoll_wait(epollfd, evs, 10, -1);

        // 调用 epoll_wait() 失败
        if (infds < 0) {
            logfile.Write("epoll() failed.\n");
            break;
        }

        // 遍历epoll_wait()函数返回的发送事件的数组evs;
        for (int i = 0; i < infds; i++) {

            ///////////////////////////////////////////////////////////////////////////////////// 
            // 如果定时器的时间已到, 向控制通道发送心跳, 设置进程的心跳, 清理空闲的客户端socket
            if (evs[i].data.fd == tfd) {
                timerfd_settime(tfd, 0, &timeout, NULL);       // 重新设置定时器

                PActive.UptATime();                            // 更新进程的心跳

                // 通过控制通道向内网程序发送心跳报文
                char buffer[256]; memset(buffer, 0, sizeof(buffer));
                strcpy(buffer, "<activetest>");
                if (send(cmdconnsock, buffer, strlen(buffer), 0) <= 0) {
                    logfile.Write("与内网程序的控制通道已断开.\n");
                    EXIT(-1);
                }

                // 清理空闲的客户端socket
                for (int i = 0; i < MAXSOCK; i++) {
                    if (clientsocks[i] > 0 && (time(NULL) - clientatime[i]) > 80) {
                        logfile.Write("client(%d,%d) timeout.\n", clientsocks[i], clientsocks[clientsocks[i]]);
                        close(clientsocks[i]);
                        close(clientsocks[clientsocks[i]]);
                        clientsocks[clientsocks[i]] = 0;
                        clientsocks[i] = 0;
                    }
                }
                continue;
            } 
            /////////////////////////////////////////////////////////////////////////////////////  

            ///////////////////////////////////////////////////////////////////////////////////// 
            // 如果发生事件的是监听外网的socket, 表示外网有新的客户端socket连接上来了
            int j = 0;
            for (j = 0; j < vroute.size(); j++) {
                if (evs[i].data.fd == vroute[j].listensock) {
                    // 接收外网客户端的连接
                    int srcsock = acceptClient(vroute[j].listensock);
                    if (srcsock < 0) break;
                    if (srcsock >= MAXSOCK) {
                        logfile.Write("连接数已超过最大值%d.\n",MAXSOCK);
                        close(srcsock);
                        break;
                    }

                    // 通过控制通道向内网程序发送命令, 把路由参数发送到内网程序
                    char buffer[256]; memset(buffer, 0, sizeof(buffer));
                    sprintf(buffer, "<dstip>%s</dstip><dstport>%d</dstport>", vroute[j].dstip, vroute[j].dstport);
                    if (send(cmdconnsock, buffer, strlen(buffer), 0) <= 0) {
                        logfile.Write("与内网的控制通道已断开.\n");
                        EXIT(-1);
                    }
                    
                    // 接收内网程序的连接
                    int dstsock = acceptClient(cmdlistensock);
                    if (dstsock < 0) break;
                    if (dstsock >= MAXSOCK) {
                        logfile.Write("连接数已超过最大值%d.\n",MAXSOCK);
                        close(srcsock);
                        close(dstsock);
                        break;
                    }

                    // 把内网程序和外网客户端对接在一起
                    // 为新连接的两个socket准备可读事件, 并添加到epollfd中
                    ev.data.fd = srcsock; ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
                    ev.data.fd = dstsock; ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

                    // 更新clientsocks数组中的socket值和clientatime数组的活动时间
                    clientsocks[srcsock] = dstsock; clientatime[srcsock] = time(NULL);
                    clientsocks[dstsock] = srcsock; clientatime[dstsock] = time(NULL);

                    logfile.Write("accept port on %d client(%d, %d) ok. \n", vroute[j].listenport, srcsock, dstsock);
                    break;
                }
            }

            // 如果jj<vroute.size(), 表示事件在上面的循环中已经处理完
            if (j < vroute.size()) continue;
            ///////////////////////////////////////////////////////////////////////////////////// 

            ///////////////////////////////////////////////////////////////////////////////////// 
            // 以下流程处理内外网通信链路socket的事件
            char buffer[1024];            // 从socket读取的数据
            int bufferlen = 0;            // 从socket中读到的数据大小
            
            // 从一端读取数据
            memset(buffer, 0, sizeof(buffer));
            if ((bufferlen= recv(evs[i].data.fd, buffer, 1000, 0)) <= 0 ) {
                // 如果连接已断开连接, 需要关闭两个通道的socket
                printf("client(%d, %d) disconnected.\n", evs[i].data.fd, clientsocks[evs[i].data.fd]);
                close(evs[i].data.fd);
                close(clientsocks[evs[i].data.fd]);

                // 重置clientsocks数值和clientatime数组
                clientsocks[clientsocks[evs[i].data.fd]] = 0;
                clientsocks[evs[i].data.fd] = 0;

                continue;
            }

            // 成功读取到了数据, 把接收到的报文原封不动的发送给对端
            // printf("from %d to %d, %d bytes.\n", evs[i].data.fd, clientsocks[evs[i].data.fd], bufferlen); 
            send(clientsocks[evs[i].data.fd], buffer, bufferlen, 0);

            // 更新两端socket连接的使用时间
            clientatime[evs[i].data.fd] = time(NULL);
            clientatime[clientsocks[evs[i].data.fd]] = time(NULL);
            ///////////////////////////////////////////////////////////////////////////////////// 
        }

    }

    return true;
}



int initServer(int port, bool isblock) {

    // 创建服务端用于监听的socket
    int listensock = socket(AF_INET, SOCK_STREAM, 0);
    if (listensock < 0) {
        perror("socket");
        return -1;
    }

    // 打开SO_REUSEADDR选项，当服务端连接处于TIME_WAIT状态时可以再次启动服务器，
    // 否则bind()可能会不成功，报：Address already in use。
    //char opt = 1; unsigned int len = sizeof(opt);
    int opt = 1; unsigned int len = sizeof(opt);
    setsockopt(listensock,SOL_SOCKET,SO_REUSEADDR,&opt,len); 

    // 声明服务端用于监听的IP地址和端口
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    // 把服务端用于监听的IP地址和端口绑定到socket上
    if (bind(listensock, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        close(listensock);
        perror("bind");
        return -1;
    }

    // 把socket设置为监听模式
    if (listen(listensock, 5) < 0) {
        close(listensock);
        perror("listen");
        return -1;
    }

    // 设置监听的socket为非阻塞
    if (isblock == true)
        fcntl(listensock, F_SETFL, fcntl(listensock, F_GETFD, 0) | O_NONBLOCK);

    return listensock;
}


bool loadRoute(const char *inifile) {

    CFile     File;       // 文件操作类
    CCmdStr   CmdStr;     // 字符串拆分类

    struct st_route stroute;

    // 打开文件
    if (!File.Open(inifile, "r")) {
        logfile.Write("File.Open(%s) failed.\n", inifile);
        return false;
    }

    char strBuffer[1024];
    while (true) {
        memset(strBuffer, 0, sizeof(strBuffer));
        memset(&stroute, 0, sizeof(stroute));

        // 读取文件内容
        if (!File.Fgets(strBuffer ,1000)) break;

        // 去掉注释内容
        char *pos = strstr(strBuffer, "#");
        if (pos != NULL) pos[0] = '\0'; 
        
        // 整理格式
        DeleteRChar(strBuffer, ' ');              // 删除字符串右边的空格
        UpdateStr(strBuffer, "  ", " ", true);    // 把两个空格替换成一个空格, 注意第三个参数

        // 拆分字符串
        CmdStr.SplitToCmd(strBuffer, " ");
        if (CmdStr.CmdCount() != 3) continue;

        // 并获取元素
        CmdStr.GetValue(0, &stroute.listenport);
        CmdStr.GetValue(1, stroute.dstip, 30);
        CmdStr.GetValue(2, &stroute.dstport);

        // 把代理参数加载到vroute容器中
        vroute.push_back(stroute);
    }

    return true;
}


int acceptClient(const int listenfd, bool isblock) {

    struct sockaddr_in clientaddr;          // 客户端的地址信息
    socklen_t len = sizeof(clientaddr);     // struct sockaddr_in的大小

    // 接收客户端的连接
    int sockfd = accept(listenfd, (struct sockaddr *)&clientaddr, &len);
    if (sockfd < 0) {
        perror("accept");
        return -1;
    }

    // 设置监听的socket为非阻塞
    if (isblock == true)
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

    return sockfd;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);

    // 关闭监听内网程序的socket
    close(cmdlistensock);

    // 关闭内网程序与外网程序的控制通道
    close(cmdconnsock);

    // 关闭全部监听的socket
    for (int i = 0; i < vroute.size(); i++) {
        close(vroute[i].listenport);
    }

    // 关闭客户端的socket
    for (int i = 0; i < MAXSOCK; i++) 
        if (clientsocks[i] > 0) close(clientsocks[i]);

    // 关闭epoll句柄
    close(epollfd);

    // 关闭定时器句柄
    close(tfd);

    exit(0);
}