/*
 *  程序名: book10.cpp, 本程序演示线程安全
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


volatile int var = 0;


// 线程主函数
void *thmain(void *);




int main(int argc, char const *argv[])
{
    pthread_t thid1 = 0, thid2 = 0;

    // 创建线程
    pthread_create(&thid1, NULL, thmain, NULL);
    pthread_create(&thid2, NULL, thmain, NULL);


    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join ok.\n");

    printf("var = %d\n", var);

    return 0;
}



void *thmain(void *) {

    for (int i = 0; i < 1000000; i++) {
        var++;
    }

    return (void *)0;
}