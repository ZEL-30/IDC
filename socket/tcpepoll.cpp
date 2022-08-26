/*
 *  程序名: tcpepoll.cpp, 此程序用于演示采用epoll模型的使用方法
 *  作者: ZEL
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>     
#include <arpa/inet.h>
#include <sys/fcntl.h>
#include <sys/epoll.h>




// 程序的帮助文档
void _help();

// 初始化服务端用于监听的socket
int initServer(int port);




int main(int argc, char const *argv[]) {
    
    if (argc != 2) {
        _help();
        return -1;
    }

    // 初始化服务端用于监听的socket
    int listensock = initServer(atoi(argv[1]));
    if (listensock < 0) {
        printf("initServer() failed.\n");
        return -1;
    }

    // 创建一个epoll句柄
    int epollfd = epoll_create(1);

    // 为监听的socket准备可读事件
    struct epoll_event ev;      // 声明事件的数据结构
    ev.events = EPOLLIN;        // 关心读事件
    ev.data.fd = listensock;    // 指定事件的自定义数据, 会随着epoll_wait()返回的事件一并返回

    // 注册事件, 把监听的socket的事件加入epollfd中
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listensock, &ev) == -1) {
        printf("epoll_ctl() failed.\n");
        return -1;
    }

    struct epoll_event evs[10];   // 存放epoll返回的事件

    while (true) {

        // 等待事件的发生
        int infds = epoll_wait(epollfd, evs, 10, -1);

        // 调用失败
        if (infds < 0) {
            printf("epoll_wait() failed.\n");
            break;
        }

        // 调用超时
        if (infds == 0) {
            printf("epoll_wait() timeout.\n");
            continue;
        }

        // 如果infds > 0, 表示有事件发生的socket的数量 
        if (infds > 0) {

            // 遍历epoll返回的已发生事件的数组evs
            for (int i = 0; i < infds; i++) {

                // 如果发生的事件是listensock, 表示有新的客户端连上来了
                if (evs[i].data.fd == listensock) {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int clientsock = accept(listensock, (sockaddr *)&clientaddr, &len);
                    printf("accept client(%d) ok.\n", clientsock);
                
                    // 为新客户端准备可读事件, 并添加到epoll中
                    ev.data.fd = clientsock;
                    ev.events = EPOLLIN;
                    epoll_ctl(epollfd, EPOLL_CTL_ADD, clientsock, &ev);

                } else {
                    // 如果发生的事件是cliensock, 表示收到客户端的报文或者客户端已断开
                    char buffer[1024]; memset(buffer, 0, sizeof(buffer));
                    if (recv(evs[i].data.fd, buffer, sizeof(buffer), 0) <= 0) {
                        // 客户端已断开
                        printf("client(eventfd = %d) disconnected.\n", evs[i].data.fd);
                        close(evs[i].data.fd);
                    } else {
                        // 客户端有报文发过来
                        printf("recv(client = %d) : %s\n", evs[i].data.fd, buffer);
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, "ok");
                        send(evs[i].data.fd, buffer, strlen(buffer), 0);
                    }

                }

            } 

        }

    }


    return 0;
}



void _help() {

    printf("\n");
    printf("Using : ./tcpepoll.out port\n");
    printf("Example : ./tcpepoll.out 5010\n");
}


int initServer(int port) {

    int listenfd = socket(AF_INET, SOCK_STREAM, NULL);
    if (listenfd < 0) {
        perror("socket");
        return -1;
    }
    
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        return -1;
    }
 
    if (listen(listenfd, 5) == -1) {
        perror("listen");
        return -1;
    }

    return listenfd;
}



