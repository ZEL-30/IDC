/*
 *  程序名: rinetdin.cpp, 网络代理(反向)服务程序-内网端
 *  作者: ZEL
 */

#include "_public.h"

#define  MAXSOCK  1024

CLogFile  logfile;       // 日志文件操作类
CPActive  PActive;       // 进程心跳操作类



int  clientsocks[MAXSOCK];            // 存放每个连接对端的socket的值
int  clientatime[MAXSOCK];            // 存放没给连接最后一次收发报文的时间
int  cmdconnsock = 0;                 // 内网程序与外网程序的控制通道
int  epollfd = 0;                     // epoll句柄
int  tfd = 0;                         // 定时器句柄



// 程序帮助文档
void _help();

// 网络代理程序主函数
bool _rinetdin(const char *ip, const int port);

// 接收客户端的新连接
int acceptClient(const int listenfd, bool isblock = false);

// 向目标IP和端口发起连接
int connToDst(const char *ip, const int port, bool isblock = false);

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
    PActive.AddPInfo(30, "rinetdin"); 

    // 网络代理程序主函数
    if (!_rinetdin(argv[2], atoi(argv[3]))) {
        logfile.Write("_rinetdin() failed.\n");
        EXIT(-1);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using : ./rinetdin.out logfile ip port\n");
    printf("Example : ./rinetdin.out /tmp/rinetdin.log 124.70.51.197 5000\n\n");
    printf("          /project/tools/bin/procctl 5 /project/tools/bin/rinetdin /tmp/rinetdin.log 124.70.51.197 5000\n\n");
    printf("logfile 本程序运行的日志文件名\n");
    printf("ip      外网代理服务端的地址\n");
    printf("port    外网代理服务端的端口\n\n\n");
}


bool _rinetdin(const char *ip, const int port) {

    // 建立内网程序和外网程序的控制通道
    if ((cmdconnsock = connToDst(ip, port, true)) < 0) {
        logfile.Write("connToDst(%s, %d) failed.\n", ip, port);
        return false;
    }
    logfile.Write("与外部的控制通道已建立(cmdconnsock = %d).\n", cmdconnsock);

    // 创建epoll句柄
    epollfd = epoll_create(1);
    
    // 为控制通道准备可读事件
    struct epoll_event ev;       // 声明事件的数据结构
    ev.data.fd = cmdconnsock;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, cmdconnsock, &ev);
   
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

    struct epoll_event evs[10];        // 声明epoll_wait()函数返回的事件的数据结构

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
            // 如果定时器的时间已到, 设置进程的心跳, 清理空闲的客户端socket
            if (evs[i].data.fd == tfd) {
                timerfd_settime(tfd, 0, &timeout, NULL);       // 重新设置定时器

                PActive.UptATime();                             // 更新进程的心跳

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
            // 如果发生事件的是控制通道
            if (evs[i].data.fd == cmdconnsock) {
                // 读取控制报文内容
                char buffer[256]; memset(buffer, 0, sizeof(buffer));
                if (recv(cmdconnsock, buffer, 200, 0) <= 0) {
                    logfile.Write("与外网的控制通道已断开.\n"); 
                    EXIT(-1);
                }

                // 如果收到的是心跳报文
                if (strcmp(buffer, "<activetest>") == 0) continue;

                // 如果收到的是新建连接的命令, 向外网服务端发起连接请求
                int srcsock = connToDst(ip, port, true);
                if (srcsock < 0) continue;
                if (srcsock >= MAXSOCK) {
                    logfile.Write("连接数已超过最大值%d.\n", MAXSOCK);
                    close(srcsock);
                    continue;
                }

                // 从控制通道发来的报文中获取目标IP和端口
                char dstip[31]; memset(dstip, 0, sizeof(dstip));
                int  dstport = 0;
                GetXMLBuffer(buffer, "dstip", dstip, 30);
                GetXMLBuffer(buffer, "dstport", &dstport);

                // 向目标服务地址和端口发起socket连接
                int dstsock = connToDst(dstip, dstport, true);
                if (dstsock < 0) continue;
                if (dstsock >= MAXSOCK) {
                    logfile.Write("连接数已超过最大值%d.\n", MAXSOCK);
                    close(srcsock);
                    close(dstsock);
                    continue;
                }
                logfile.Write("新建内外网通道(%d, %d) ok.\n", srcsock, dstsock);

                // 把内网和外网的socket对接在一起
                ev.data.fd = srcsock; ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, srcsock, &ev);
                ev.data.fd = dstsock; ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, dstsock, &ev);

                clientsocks[srcsock] = dstsock; clientatime[srcsock] = time(NULL);
                clientsocks[dstsock] = srcsock; clientatime[dstsock] = time(NULL);

                continue;
            }
            /////////////////////////////////////////////////////////////////////////////////////

            ///////////////////////////////////////////////////////////////////////////////////// 
            // 如果发生事件的是客户端连接的socket, 表示客户端发送过来了报文或者是客户端已断开
            char buffer[1024];            // 存放从客户端读取的数据
            int bufferlen = 0;            // 存放从socket中读到的数据大小
            
            memset(buffer, 0, sizeof(buffer));
            if ((bufferlen= recv(evs[i].data.fd, buffer, 1000, 0)) <= 0 ) {
                // 如果是客户端断开连接
                printf("client(%d, %d) disconnected.\n", evs[i].data.fd, clientsocks[evs[i].data.fd]);
                close(evs[i].data.fd);
                close(clientsocks[evs[i].data.fd]);

                // 重置clientsocks数值和clientatime数组
                clientsocks[clientsocks[evs[i].data.fd]] = 0;
                clientsocks[evs[i].data.fd] = 0;
            }

            // 如果是客户端发过来了报文, 把报文原封不动的发送给对端
            // printf("from %d to %d, %d bytes.\n", evs[i].data.fd, clientsocks[evs[i].data.fd], bufferlen); 
            send(clientsocks[evs[i].data.fd], buffer, bufferlen, 0);

            // 更新客户端连接的使用时间
            clientatime[evs[i].data.fd] = time(NULL);
            clientatime[clientsocks[evs[i].data.fd]] = time(NULL);
            ///////////////////////////////////////////////////////////////////////////////////// 
        }

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

    // 设置新客户端的socket为非阻塞
    if (isblock == true)
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

    return sockfd;
}


int connToDst(const char *ip, const int port, bool isblock) {

    // 创建用于连接目标IP和端口的socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    // 准备用于绑定的IP
    struct hostent *h;
    if ((h = gethostbyname(ip)) == 0) {
        close(sockfd);
        return -1;
    }

    // 准备用于绑定的端口和IP
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

    // 向目标IP和端口发起连接请求
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    // 设置socket为非阻塞
    if (isblock == true)
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFD, 0) | O_NONBLOCK);

    return sockfd;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);

    // 关闭内网程序与外网程序的控制通道
    close(cmdconnsock);
    
    // 关闭客户端的socket
    for (int i = 0; i < MAXSOCK; i++) 
        if (clientsocks[i] > 0) close(clientsocks[i]);

    // 关闭epoll句柄
    close(epollfd);

    // 关闭定时器句柄
    close(tfd);

    exit(0);
}