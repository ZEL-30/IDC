/*
 *  程序名: demo21.cpp, 本程序采用多进程实现网络通信的客户端
 *  作者: ZEL
 */

#include "_public.h"

CTcpClient TcpClient;     // 网络通信客户端类


char strsendbuffer[1024];      // 发送报文的buffer
char strrecvbuffer[1024];      // 接受报文的buffer


// 程序的帮助文档
void _help();

// 线程一主函数, 用于向服务端发送报文
void *thmain1(void *arg);

// 线程二主函数, 用于接受服务端的回应报文
void *thmain2(void *arg);





int main(int argc, char const *argv[]) {
    
    if (argc != 3) {
        _help();
        return -1;
    }

    // 连接服务端
    if (!TcpClient.ConnectToServer(argv[1], atoi(argv[2]))) {
        printf("TcpClient.ConnectToServer(%s, %s) failed.\n", argv[1], argv[2]);
        return -1;
    }

    // 异步通信(多线程)与服务端通信
    pthread_t thid1 = 0, thid2 = 0;
    pthread_create(&thid1, NULL, thmain1, NULL);
    pthread_create(&thid2, NULL, thmain2, NULL);


    // 等待子线程退出
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);


    return 0;
}



void _help() {

    printf("\n");
    printf("Using : ./demo21.out ip port\n");
    printf("Example : ./demo21.out 124.70.51.197 5005\n\n");
}


void *thmain1(void *arg) {

    for (int i = 0; i < 50; i++) {

        // 向服务端发送报文
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        sprintf(strsendbuffer, "%2d勇士总冠军!!!", i + 1);
        if (!TcpClient.Write(strsendbuffer)) break;
        printf("发送 : %s\n", strsendbuffer);
    }

    return NULL;
}

void *thmain2(void *arg) {

    for (int i = 0; i < 50; i++) {
        // 接受服务端的回应报文
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        if (!TcpClient.Read(strrecvbuffer)) break;
        printf("接受 : %s\n", strrecvbuffer);
    }

    return NULL;
}