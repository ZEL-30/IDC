/*
 *  程序名: tcpselect.cpp, 此程序用于演示采用select模型的使用方法
 *  作者: ZEL
 */
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
    printf("listensock=%d\n",listensock);
    if (listensock < 0) {
        printf("initServer() failed.\n");
        return -1;
    }

    fd_set readfds;                  // 读事件sockdt的集合, 包括监听socket和客户端连接上来的socket
    FD_ZERO(&readfds);               // 初始化读事件socket的集合
    FD_SET(listensock, &readfds);    // 把listensock添加到读事件socket的集合中
    
    int maxfd = listensock;          // 记录集合中socket的最大值
    
    while (true) {

        fd_set tmpfds = readfds;     // 复制一个读事件的副本, 传给select函数
        struct timeval timeout;
        timeout.tv_sec = 10; timeout.tv_usec = 0;
        int infds = select(maxfd + 1, &tmpfds, NULL, NULL, &timeout);

        // 调用失败
        if (infds < 0) {
            printf("select() failed.\n");
            perror("select");
            break;
        }

        // 超时, 在本程序中, select函数最后一个参数为空, 不存在超时的情况, 但以下代码还是留着
        if (infds ==  0) {
            printf("select() timeout.\n");
            continue;
        }

        // 如果infds大于零, 表示有事件发生的数量 
        if (infds > 0) {
            for (int eventfd = 0; eventfd <= maxfd; eventfd++) {
                // 如果没有事件, 继续循环
                if (FD_ISSET(eventfd, &tmpfds) <= 0) continue;

                // 如果发生事件的是listensock, 表示有新的客户端连上来
                if (eventfd == listensock) {
                    struct sockaddr_in clientaddr;
                    socklen_t len = sizeof(clientaddr);
                    int clientsock = accept(listensock, (struct sockaddr *)&clientaddr, &len);
                    if (clientsock < 0) {
                        perror("accept");
                        continue;
                    }
                    printf ("accept client(socket=%d) ok.\n",clientsock);

                    // 把新客户端的socket加入可读socket的集合
                    FD_SET(clientsock, &readfds);

                    // 更新maxfd的值
                    if (maxfd < clientsock) maxfd = clientsock;

                } else {
                    // 如果是客户端连接的socket有事件, 表示有报文发过来或者连接已断开
                    char buffer[1024]; memset(buffer, 0, sizeof(buffer));
                    if (recv(eventfd, buffer, sizeof(buffer), 0) <= 0) {
                        // 如果客户端的连接已断开
                        printf("client(eventfd = %d) disconnected.\n", eventfd);
                        close(eventfd);
                        FD_CLR(eventfd, &readfds);

                        // 重新计算maxfd的值，注意，只有当eventfd==maxfd时才需要计算。
                        if (eventfd == maxfd) {
                            // 从后面往前找
                            for (int ii=maxfd;ii>0;ii--) {
                                if (FD_ISSET(ii,&readfds)) {
                                    maxfd = ii; break;
                                }
                            }
                        }
                    } else {
                        // 接收客户端发来的请求报文
                        printf("recv(eventfd=%d):%s\n",eventfd,buffer);

                        // 把接收到的报文内容原封不动的发回去。
                        fd_set tmpfds;
                        FD_ZERO(&tmpfds);
                        FD_SET(eventfd, &tmpfds);
                        if (select(eventfd+1,NULL, &tmpfds, NULL, NULL) <= 0)
                            perror("select() failed");
                        else
                            send(eventfd,buffer,strlen(buffer),0);
                    }

                }
            }
        }
    }


    return 0;
}



void _help() {

    printf("\n");
    printf("Using : ./tcpselect.out port\n");
    printf("Example : ./tcpselect.out 5010\n");
}


int initServer(int port) {

    int listenfd = socket(AF_INET, SOCK_STREAM, NULL);
    if (listenfd == -1) {
        perror("socket");
        return -1;
    } 

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(struct sockaddr_in));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("bind");
        close(listenfd);
        return -1;
    }

    if (listen(listenfd, 10) != 0) {
        perror("listen");
        close(listenfd);
        return -1;
    }

    return listenfd;
}



