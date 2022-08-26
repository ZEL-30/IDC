/*
 *  程序名: book06.cpp, 本程序演示线程资源的回收
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void *thmain1(void *arg);
void *thmain2(void *arg);

int main(int argc, char *argv[]) {

    pthread_t thid1 = 0, thid2 = 0;
    pthread_create(&thid1, NULL, thmain1, NULL);
    pthread_detach(thid1);



    pthread_create(&thid2, NULL, thmain2, NULL);

    sleep(10);

    void *ret = new int;
    pthread_join(thid1, &ret);
    printf("ret1 = %d\n", (int)(long)ret);


    pthread_join(thid2, &ret);
    printf("ret2 = %d\n", (int)(long)ret);


    return 0;
}




void *thmain1(void *arg) {
    printf("线程%lu开始运行.\n", pthread_self());

    for (int i = 0; i < 3; i++) {
        sleep(1);
        printf("thmain1 sleep(%d)\n", i + 1);
    }


    pthread_exit((void *)1);
}

void *thmain2(void *arg) {
    printf("线程%lu开始运行.\n", pthread_self());

    for (int i = 0; i < 5; i++) {
        sleep(1);
        printf("thmain1 sleep(%d)\n", i + 1);
    }


    pthread_exit((void *)2);
}