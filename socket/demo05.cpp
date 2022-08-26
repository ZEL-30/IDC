/*
 *  程序名: demo05.cpp, 本程序用于演示不粘包的socket通信的客户端
 *  作者: ZEL
 */
#include "_public.h"


// 程序帮助文档
void _help();





int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    
    // 第一步: 创建客户端的sock
    int sockfd;
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return -1;
    }

    // 指定ip
    struct hostent *h;
    if ((h = gethostbyname(argv[1])) == 0) { 
        printf("gethostbyname failed.\n"); 
        close(sockfd); 
        return -1; 
    }

    // 指定协议族和端口
    struct sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(atoi(argv[2]));  // 把端口转换为网络字节序
    memcpy(&servaddr.sin_addr, h->h_addr, h->h_length);

    // 第二步: 向服务器发起连接请求
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) != 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }

    // 第三步: 与服务端通信，发送一个报文后等待回复，然后再发下一个报文
    char buffer[1024];
    for (int i = 0; i < 50000; i++) {

        // 向服务端发起请求报文
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, "勇士总冠军!!!");
        if (!TcpWrite(sockfd, buffer, strlen(buffer))) break;
        printf("发送: %s\n", buffer);

    }

    // 第四步: 关闭socket,释放资源
    close(sockfd);

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo05 ip port\nExample:./demo05.out 127.0.0.1 5005\n\n"); 
}
