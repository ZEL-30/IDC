/*
 *  程序名: book09.cpp, 本程序演示线程和信号
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


// 线程主函数
void *thmain(void *);

// 信号处理函数
void func(int sig) {
    printf("catch signal %d\n", sig);
}



int main(int argc, char const *argv[])
{

    signal(2, func);


    pthread_t thid = 0;

    // 创建线程
    pthread_create(&thid, NULL, thmain, NULL);

    // 主进程向子线程发送信号
    sleep(5);
    pthread_kill(thid, 2);

    printf("join ... \n");
    pthread_join(thid, NULL);
    printf("join ok.\n");

    return 0;
}



void *thmain(void *) {

    printf("sleep ....\n");
    sleep(10);
    printf("sleep ok.\n");

    return (void *)1;
}