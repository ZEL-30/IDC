/*
 *  程序名: book11.cpp, 本程序演示线程同步-互斥锁
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int var = 0;


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // 声明互斥锁


// 线程主函数
void *thmain(void *);




int main(int argc, char const *argv[])
{

    // 互斥锁初始化
    // pthread_mutex_init(&mutex, NULL);


    pthread_t thid1 = 0, thid2 = 0;

    // 创建线程
    pthread_create(&thid1, NULL, thmain, NULL);
    pthread_create(&thid2, NULL, thmain, NULL);


    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join ok.\n");

    printf("var = %d\n", var);

    // 销毁互斥锁
    pthread_mutex_destroy(&mutex);

    return 0;
}



void *thmain(void *) {

    // 互斥锁加锁
    pthread_mutex_lock(&mutex);

    for (int i = 0; i < 1000000; i++) {
        var++;
    }

    // 互斥锁解锁
    pthread_mutex_unlock(&mutex);

    return (void *)0;
}