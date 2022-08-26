/*
 *  程序名: demo08.cpp, 本程序用于演示采用TcpClient类实现socket通信的服务端
 *  作者: ZEL
 */
#include "_public.h"


CTcpServer  TcpServer;  // socket通讯的服务端类


// 程序帮助文档
void _help();





int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 2) {
        _help();
        return -1;
    }

    // 服务端初始化
    if (!TcpServer.InitServer(atoi(argv[1]))) {
        printf("TcpClient.InitServerr(%s) failed.\n",argv[1]);
        return -1;
    }

    // 接收客户端的连接请求
    if (!TcpServer.Accept()) {
        printf("TcpClient.Accept() failed.\n");
        return -1;
    }
    
    // 与客户端通信，接收客户端发过来的报文后，回复ok
    char buffer[102400];
    while (true) {
        // 接收客户端的请求报文
        memset(buffer, 0, sizeof(buffer));
        if (!TcpServer.Read(buffer)) break;
        printf("接收：%s\n", buffer);

        // 向客户端发送回应报文
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer,"ok");
        if (!TcpServer.Write(buffer, strlen(buffer))) break;
        printf("发送：%s\n", buffer);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo08 port\nExample:./demo08.out 5005\n\n");
}
