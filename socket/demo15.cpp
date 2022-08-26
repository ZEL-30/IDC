/*
 *  程序名: demo15.cpp, 本程序采用多进程的方式实现网络通信的客户端
 *  作者: ZEL
 */
#include "_public.h"


CTcpClient TcpClient;  // socket通讯的客户端类


// 程序帮助文档
void _help();





int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    // 向服务端发起连接请求
    if (!TcpClient.ConnectToServer(argv[1], atoi(argv[2]))) {
      printf("TcpClient.ConnectToServer(%s,%s) failed.\n", argv[1], argv[2]);
      return -1;
    }

    // 采用异步通信(多进程), 父进程只发送报文, 子进程只接收报文
    int pid = fork();

    char buffer[1024];
    for (int i = 0; i < 1000; i++) {
        if (pid > 0) {
            // 父进程向服务端发送请求报文
            memset(buffer, 0, sizeof(buffer));
            SPRINTF(buffer, sizeof(buffer), "勇士总冠军!!!%d", i);
            if (!TcpClient.Write(buffer, strlen(buffer))) break;
            printf("发送：%s\n", buffer);
        } else {
            // 子进程接收服务端的回应报文
            memset(buffer, 0, sizeof(buffer));
            if (!TcpClient.Read(buffer)) break;
            printf("接收：%s\n", buffer);
        }
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo15 ip port\nExample:./demo15.out 124.70.51.197 5005\n\n"); 
}
