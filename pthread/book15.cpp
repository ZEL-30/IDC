/*
 *  程序名: book15.cpp, 本程序演示线程同步-信号量
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>



int var = 0;


sem_t sem;  // 声明信号量


// 线程主函数
void *thmain(void *);




int main(int argc, char const *argv[])
{

    // 信号量初始化
    sem_init(&sem, 0, 1);


    pthread_t thid1 = 0, thid2 = 0;

    // 创建线程
    pthread_create(&thid1, NULL, thmain, NULL);
    pthread_create(&thid2, NULL, thmain, NULL);


    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join ok.\n");

    printf("var = %d\n", var);

    // 销毁信号量
    sem_destroy(&sem);

    return 0;
}



void *thmain(void *) {

    // 信号量P操作
    sem_wait(&sem);

    for (int i = 0; i < 1000000; i++) {
        var++;
    }

    // 信号量V操作
    sem_post(&sem);

    return (void *)0;
}