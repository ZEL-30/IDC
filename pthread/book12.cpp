/*
 *  程序名: book12.cpp, 本程序演示线程同步-自旋锁
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int var = 0;


pthread_spinlock_t spin;  // 声明自旋锁


// 线程主函数
void *thmain(void *);




int main(int argc, char const *argv[])
{

    // 自旋锁初始化
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);


    pthread_t thid1 = 0, thid2 = 0;

    // 创建线程
    pthread_create(&thid1, NULL, thmain, NULL);
    pthread_create(&thid2, NULL, thmain, NULL);


    printf("join ... \n");
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    printf("join ok.\n");

    printf("var = %d\n", var);

    // 销毁自旋锁
    pthread_spin_destroy(&spin);

    return 0;
}



void *thmain(void *) {

    // 自旋锁加锁
    pthread_spin_lock(&spin);

    for (int i = 0; i < 1000000; i++) {
        var++;
    }

    // 自旋锁解锁
    pthread_spin_unlock(&spin);

    return (void *)0;
}