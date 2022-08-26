/*
 *  程序名: demo17.cpp, 本程序采用多线程的方式实现网络通信的客户端
 *  作者: ZEL
 */
#include "_public.h"


CTcpClient TcpClient;  // socket通讯的客户端类


// 程序帮助文档
void _help();

// 线程一主函数
void *thmain1(void *arg);

// 线程二主函数
void *thmain2(void *arg);





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

    // 采用异步通信(多线程), 线程一只发送报文, 线程二只接收报文
    pthread_t thid1, thid2 = 0;
    pthread_create(&thid1, NULL, thmain1, NULL);
    pthread_create(&thid2, NULL, thmain2, NULL);

    //  主进程等待线程退出
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo17 ip port\nExample:./demo17.out 124.70.51.197 5005\n\n"); 
}


void *thmain1(void *arg) {

    char buffer[1024];
    for (int i = 0; i < 1000; i++) {
        // 线程一向服务端发送请求报文
        memset(buffer, 0, sizeof(buffer));
        SPRINTF(buffer, sizeof(buffer), "勇士总冠军!!!%d", i);
        if (!TcpClient.Write(buffer, strlen(buffer))) break;
        printf("发送：%s\n", buffer);
    }

    return 0;
}

void *thmain2(void *arg) {

    char buffer[1024];
    for (int i = 0; i < 1000; i++) {
        // 线程二接收服务端的回应报文
        memset(buffer, 0, sizeof(buffer));
        if (!TcpClient.Read(buffer)) return 0;
        printf("接收：%s\n", buffer);
    }

    return 0;
}