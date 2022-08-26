/*
 *  程序名: demo22.cpp, 本程序采用开发框架的CTcpServer类和多线程的方式实现网络通信的服务端
 *  作者: ZEL
 */
#include "_public.h"

CTcpServer TcpServer;     // 网络通信服务端类
CLogFile   logfile;       // 日志文件操作类

pthread_spinlock_t spin;       // 声明用于给vthid容器加锁的自旋锁
vector<pthread_t> vthid;       // 存放所有的线程ID

char strsendbuffer[1024];      // 发送报文的buffer
char strrecvbuffer[1024];      // 接受报文的buffer


// 程序的帮助文档
void _help();

// 线程主函数
void *thmain(void *arg);

// 线程清理函数
void thcleanup(void *arg);

// 进程的信号处理函数
void EXIT(int sig);





int main(int argc, char const *argv[]) {
    
    if (argc != 3) {
        _help();
        return -1;
    }

    // 打开日志文件
    if (!logfile.Open(argv[2], "a")) {
        printf("logfile.Open(%d) failed.\n", argv[2]);
        return -1;
    }

    CloseIOAndSignal();
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 初始化自旋锁
    pthread_spin_init(&spin, 0);

    // 初始化服务端
    if (!TcpServer.InitServer(atoi(argv[1]))) {
        logfile.Write("TcpServer.InitServer(%s) failed.\n", argv[1]);
        return -1;
    }

    while (true) {

        // 等待客户端连接
        if (!TcpServer.Accept()) {
            logfile.Write("TcpServer.Accept() failed.\n");
            return -1;
        }
        logfile.Write("客户端(%s)已连接\n", TcpServer.GetIP());

        // 创建线程
        pthread_t thid = 0;
        if (pthread_create(&thid, NULL, thmain, (void *)(long)TcpServer.m_connfd) != 0) {
            logfile.Write("pthread_create() failed.\n");
            TcpServer.CloseClient();
            continue;
        }

        pthread_spin_lock(&spin);        // 加锁
        vthid.push_back(thid);
        pthread_spin_unlock(&spin);      // 解锁
    }

    return 0;
}



void _help() {

    printf("\n");
    printf("Using : ./demo22.out port logfile\n");
    printf("Example : ./demo22.out 5005 /tmp/demo22.log\n\n");
}


void *thmain(void *arg) {

    // 注册线程清理函数
    pthread_cleanup_push(thcleanup, arg);

    // 设置线程的取消方式为立即取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // 分离线程, 让系统回收资源
    pthread_detach(pthread_self());

    int sockfd = (int)(long)arg;

    // 与服务端通信
    while (true) {
        // 接受服务端的回应报文
        int ibuflen = 0;
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        if (!TcpRead(sockfd, strrecvbuffer, &ibuflen, 30)) break;
        logfile.Write("接受 : %s\n", strrecvbuffer);


        // 向服务端发送报文
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        strcpy(strsendbuffer, "ok.");
        if (!TcpWrite(sockfd, strsendbuffer)) break;
        logfile.Write("发送 : %s\n", strsendbuffer);
    }

    // 关闭客户端的连接
    close(sockfd);

    // 自身的线程ID从vthid容器中删除
    pthread_spin_lock(&spin);           // 加锁
    for (int i = 0; i < vthid.size(); i++) {
        if (pthread_equal(vthid[i], pthread_self()) == 0) {
            vthid.erase(vthid.begin() + i);
            break;
        }
    }
    pthread_spin_unlock(&spin);         // 解锁

    // 线程清理函数出栈
    pthread_cleanup_pop(1);

    return NULL;    
}


void thcleanup(void *arg) {

    // 关闭客户端的socket
    int sockfd = (int)(long)arg;
    close(sockfd);

    logfile.Write("线程%lu退出。\n",pthread_self());
}


void EXIT(int sig) {

    logfile.Write("程序退出, sig = %d\n", sig);

    // 关闭监听的socket
    TcpServer.CloseListen();

    printf("vthid.size() = %d\n", vthid.size());

    // 取消全部的线程
    for (int i = 0; i < vthid.size(); i++) {
        pthread_cancel(vthid[i]);
    }

    // 让子线程有足够的时间退出
    sleep(1);

    // 释放自旋锁
    pthread_spin_destroy(&spin);
    
    exit(0);
}