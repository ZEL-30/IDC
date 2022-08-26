/*
 *  程序名: tcppoll.cpp, 此程序用于演示采用poll模型的使用方法
 *  作者: ZEL
 */
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


#define MAXNFDS 1024


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
    printf("listensock = %d.\n", listensock);
    if (listensock < 0) {
        printf("initServer() failed.\n");
        return -1;
    } 

    struct pollfd fds[MAXNFDS];     // fds存放需要监视的socket
    // 初始化数组, 把全部的fd设置为-1
    for (int i = 0; i < MAXNFDS; i++)
        fds[i].fd = -1;

    // 把listensock和读事件添加到数组中
    fds[listensock].fd = listensock;
    fds[listensock].events = POLLIN;

    int maxfd = listensock;       // fds数组中需要监视的socket大小
    
    while (true) {

        int infds = poll(fds, maxfd + 1, 5000);

        // 调用失败
        if (infds < 0) {
            printf("poll() failed.\n");
            break;
        }

        // 超时
        if (infds == 0) {
            printf("poll() timeout.\n");
            continue;
        }

        // 调用成功, 表示有事件发送的socket的数量
        if (infds > 0) {
             // 这里是客户端的socket事件, 每次都有遍历整个数组, 因为可能有多个socket有事件
            for (int eventfd = 0; eventfd <= maxfd; eventfd++) {

                // 如果fd为负数, 忽略这个数组
                if (fds[eventfd].fd < 0 ) continue;

                // 如果没有事件, continue
                if ((fds[eventfd].revents&POLLIN) == 0) continue;

                // 先把revent清空
                fds[eventfd].revents = 0;

                // 如果发生的事件是listensock, 表示有新的客户端连上来
                if (eventfd == listensock) {

                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int clientsock = accept(listensock, (struct sockaddr *)&clientaddr, &len);
                    if (clientsock < 0) {
                        perror("accept");
                        continue;
                    }
                    printf ("accept client(socket=%d) ok.\n",clientsock);

                    // 修改fds数组中clientsock的位置的元素
                    fds[clientsock].fd = clientsock;
                    fds[clientsock].events = POLLIN;
                    fds[clientsock].revents = 0;

                    // 更新maxfd的值
                    if (maxfd < clientsock) maxfd = clientsock;

                } else {
                    // 如果是客户端连接的socket事件, 表示有报文或者连接已断开

                    char buffer[1024]; memset(buffer, 0, sizeof(buffer));
                    if (recv(eventfd, buffer, sizeof(buffer), 0) <= 0) {
                        // 如果客户端的连接已断开
                        printf("client(evenfd = %d) disconnetfd.\n", eventfd);
                        close(eventfd);
                        fds[eventfd].fd = -1;

                        // 重新计算maxfd的值, 注意, 只有当eventfd == maxfd 时才需要计算
                        if (eventfd == maxfd) {
                            for (int i = maxfd; i > 0; i--) {
                                if (fds[i].fd != -1) {
                                    maxfd = i; break;
                                }
                            }
                        }
                    } else {
                        printf("recv(eventfd = %d) = %s\n", eventfd, buffer);

                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, "ok.");
                        send(eventfd, buffer, strlen(buffer), 0);
                    }
                }                   

            }
        }

    }


    return 0;
}



void _help() {

    printf("\n");
    printf("Using : ./tcppoll.out port\n");
    printf("Example : ./tcppoll.out 5010\n");
}


int initServer(int port) {

    int listensock = socket(AF_INET, SOCK_STREAM, NULL);
    if (listensock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listensock, (sockaddr *)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        return -1;
    }

    if (listen(listensock, 5) == -1) {
        perror("listen");
        return -1;
    }
   
   return listensock;
}



