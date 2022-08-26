/*
 *  程序名: demo04.cpp, 本程序用于演示粘包的socket通信的服务端
 *  作者: ZEL
 */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// 程序帮助文档
void _help();





int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 2) {
        _help();
        return -1;
    }

    // 第一步: 创建服务端的socket
    int listenfd;
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    // 指定协议族、ip地址和端口
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);  // 指定任意地址
    servaddr.sin_port = htons(atoi(argv[1]));

    // 第二步: 把服务器用于通讯的ip地址和端口绑定到socket上
    if (bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    // 第三步: 把socket设为监听模式
    if (listen(listenfd, 5) != 0) {
        printf("listen");
        close(listenfd);
        return -1;
    }

    // 第四步: 接收客户端的连接
    int clientfd;
    int socklen = sizeof(struct sockaddr_in);  // // struct sockaddr_in的大小
    struct sockaddr_in clientaddr;  // 客户端的信息地址
    clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, (socklen_t *)&socklen);

    // 第五步: 与客户端通信，接收客户端发过来的报文后，回复ok
    char buffer[102400];
    while (true) {
        // 接收客户端的请求报文
        memset(buffer, 0, sizeof(buffer));
        if (recv(clientfd, buffer, sizeof(buffer),0) <= 0) break;
        printf("接收: %s\n",buffer);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo04 port\nExample:./demo04.out 5005\n\n");
}
