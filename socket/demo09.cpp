/*
 *  程序名: demo09.cpp, 本程序用于演示采用TcpClient类实现socket通信多进程的客户端
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

    // 与服务端通讯, 发送一个报文后等待回复, 然后再发送下一个报文
    char buffer[1024];
    for (int i = 0; i < 100; i++) {
        // 向服务端发送请求报文
        memset(buffer, 0, sizeof(buffer));
        SPRINTF(buffer, sizeof(buffer), "勇士总冠军!!!%d", i);
        if (!TcpClient.Write(buffer, strlen(buffer))) break;
        printf("发送：%s\n", buffer);

        // 接收服务端的回应报文
        memset(buffer, 0, sizeof(buffer));
        if (!TcpClient.Read(buffer)) break;
        printf("接收：%s\n", buffer);

        sleep(1);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo09 ip port\nExample:./demo09.out 127.0.0.1 5005\n\n"); 
}
