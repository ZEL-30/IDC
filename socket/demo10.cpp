/*
 *  程序名: demo10.cpp, 本程序用于演示采用TcpClient类实现socket通信多进程的服务端
 *  作者: ZEL
 * 1）在多进程的服务程序中，如果杀掉一个子进程，和这个子进程通讯的客户端会断开，但是，不
 *    会影响其它的子进程和客户端，也不会影响父进程
 * 2）如果杀掉父进程，不会影响正在通讯中的子进程，但是，新的客户端无法建立连接
 * 3）如果用killall+程序名，可以杀掉父进程和全部的子进程
 *
 * 多进程网络服务端程序退出的三种情况：
 * 1. 如果是子进程收到退出信号，该子进程断开与客户端连接的socket，然后退出
 * 2. 如果是父进程收到退出信号，父进程先关闭监听的socket，然后向全部的子进程发出退出信号
 * 3. 如果父子进程都收到退出信号，本质上与第2种情况相同 
 * 
 */
#include "_public.h"


CTcpServer  TcpServer;  // socket通讯的服务端类
CLogFile    logfile;    // 日志文件操作类


// 程序帮助文档
void _help();

// 父进程退出和信号2、15的处理函数
void FathEXIT(int sig);

// 子进程退出和信号2、15的处理函数
void ChldEXIT(int sig);






int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    // 打开日志文件
    if (!logfile.Open(argv[2], "a+")) {
        printf("logfile.Open(%s) failed.\n",argv[2]);
        return -1;
    }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
    // 但请不要用 "kill -9 +进程号" 强行终止
    CloseIOAndSignal();
    signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);

    // 服务端初始化
    if (!TcpServer.InitServer(atoi(argv[1]))) {
        logfile.Write("TcpClient.InitServerr(%s) failed.\n",argv[1]);
        return -1;
    }


    while (true) {
        // 等待客户端的连接请求
        if (!TcpServer.Accept()) {
            logfile.Write("TcpClient.Accept() failed.\n");
            FathEXIT(-1);
        }
        
        logfile.Write("客户端(%s)已连接\n",TcpServer.GetIP());

        // 创建子进程, 如果是父进程, 关闭客户端的socket, 然后continue继续监听客户端请求
        if (fork() > 0) {
            // 关闭用于监听的socket
            TcpServer.CloseClient();
            continue;
        }

        // 子进程退出信号
        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);

        // 子进程, 关闭监听的socket
        TcpServer.CloseListen();

        // 子进程与客户端通信，接收客户端发过来的报文后，回复ok
        char buffer[102400];
        while (true) {
            // 接收客户端的请求报文
            memset(buffer, 0, sizeof(buffer));
            if (!TcpServer.Read(buffer)) break;
            logfile.Write("接收：%s\n", buffer);

            // 向客户端发送回应报文
            memset(buffer, 0, sizeof(buffer));
            strcpy(buffer,"ok");
            if (!TcpServer.Write(buffer, strlen(buffer))) break;
            logfile.Write("发送：%s\n", buffer);
        }

        ChldEXIT(0);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo10 port logfile\nExample:./demo10.out 5005 /tmp/demo10.log\n\n");
}


void FathEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    // 关闭监听的socket
    TcpServer.CloseListen();

    // 向所有的子进程发送退出信号
    kill(0,SIGTERM);

    // 父进程退出
    logfile.Write("父进程退出, sig = %d\n",sig);
    exit(0);
}

void ChldEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    //关闭客户端的socket
    TcpServer.CloseClient();

    // 子进程退出
    logfile.Write("子进程退出, sig = %d\n",sig);
    exit(0);
}