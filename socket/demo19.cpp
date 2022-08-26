/*
 *  程序名: demo19.cpp, 本程序采用I/O复用的方式实现网络通信的客户端
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

    char buffer[1024];
    int  ibuflen = 0;
    int  inum = 0;      // 用于接收回应报文的个数

    // 采用异步通信(I/O复用)
    for (int i = 0; i < 10000; i++) {
        memset(buffer, 0, sizeof(buffer));
        SPRINTF(buffer, sizeof(buffer), "勇士总冠军!!!%d", i);
        if (!TcpClient.Write(buffer, strlen(buffer))) break;
        printf("发送：%s\n", buffer);

        // 接收服务端的回应报文
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            if (!TcpRead(TcpClient.m_connfd, buffer, &ibuflen, -1)) break;
            printf("接收：%s\n", buffer);
            inum++;
        }
    }

    // 接收服务端的回应报文
    while (inum < 10000) {
        memset(buffer, 0, sizeof(buffer));
        if (!TcpRead(TcpClient.m_connfd, buffer, &ibuflen)) break;
        printf("接收：%s\n", buffer);
        inum++;
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo19 ip port\nExample:./demo19.out 124.70.51.197 5005\n\n"); 
}
