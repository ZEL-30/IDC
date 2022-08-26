/*
 *  程序名: demo25.cpp, 本程序用于演示HTTP客户端
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

    // 准备报文
    char buffer[102400];
    memset(buffer, 0, sizeof(buffer));
    SPRINTF(buffer, sizeof(buffer), "GET / HTTP/1.1\r\n"
                                    "Host: %s:%s\r\n"
                                    "Connection: keep-alive\r\n"
                                    "\r\n", argv[1], argv[2]);



    // 向服务端发送请求报文
    send(TcpClient.m_connfd ,buffer, strlen(buffer), 0);
    printf("%s\n", buffer);

    // 接收服务端的回应报文
    while (true) {
        memset(buffer, 0, sizeof(buffer));
        if (recv(TcpClient.m_connfd, buffer, 102400, 0) <= 0) break;
        printf("%s\n", buffer);
    }

    printf("接收成功\n");

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo07 ip port\n");
    printf("Example:./demo25.out 127.0.0.1 5005\n\n"); 
    printf("Example:./demo25.out www.weather.com.cn 80\n\n"); 
}
