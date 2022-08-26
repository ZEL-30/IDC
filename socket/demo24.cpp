/*
 *  程序名: demo24.cpp, 本程序用于演示HTTP协议, 接收HTTP请求报文
 *  作者: ZEL
 */
#include "_public.h"


CTcpServer  TcpServer;  // socket通讯的服务端类

char strrecvbuffer[102400]; 
char strsendbuffer[102400]; 

// 程序帮助文档
void _help();

// 发送html文件
bool SendHtmlFile(const int sockfd, const char *filename);



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
    printf("客户端(%s)已连接.\n", TcpServer.GetIP());    

    // 接收HTTP客户端发送过来的报文
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    recv(TcpServer.m_connfd, strrecvbuffer, 1000, 0);
    printf("%s\n", strrecvbuffer);

    // 先把响应报文的头部发送给客户端
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "HTTP/1.1 200 OK\r\n"
                                                  "Server: webserber\r\n"
                                                  "Content-Type: text/html;charset = utf-8\r\n"
                                                  "\r\n");
                                                //   "Content-Length: 105413\r\n\r\n");
    send(TcpServer.m_connfd, strsendbuffer, strlen(strsendbuffer), 0);


    // 再把HTML文件发送给客户端
    SendHtmlFile(TcpServer.m_connfd, "SURF_ZH_20220816094604_18279.xml");


    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo24 port\nExample:./demo24.out 5005\n\n");
}


bool SendHtmlFile(const int sockfd, const char *filename) {

    CFile   File;  // 文件操作类

    int   bytes = 0;   // 本次打算发送的字节数
    char buffer[5000];

    // 打开文件
    if (!File.Open(filename, "r")) {
        printf("File.Open(%s) failed.\n", filename);
        return false;
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        if (!File.Fgets(buffer, 5000)) break;

        if (!Writen(sockfd, buffer, strlen(buffer))) {
            close(sockfd);
            return false;
        }
    }

    return true;
}