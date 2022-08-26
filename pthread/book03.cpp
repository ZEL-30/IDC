/*
 *  程序名: book03.cpp, 本程序演示线程参数的传递(不用强制类型转换的方法, 传变量的地址)
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

int var = 0;

void *thmain1(void *arg);
void *thmain2(void *arg);
void *thmain3(void *arg);
void *thmain4(void *arg);
void *thmain5(void *arg);


int main(int argc, char *argv[]) {

    pthread_t thid1, thid2, thid3, thid4, thid5;
    int *var1 = new int;
    *var1 = 1;
    pthread_create(&thid1, NULL, thmain1, var1);

    int *var2= new int;
    *var2 = 2;
    pthread_create(&thid2, NULL, thmain2, var2);

    int *var3 = new int;
    *var3 = 3;
    pthread_create(&thid3, NULL, thmain3, var3);

    int *var4 = new int;
    *var4 = 4;
    pthread_create(&thid4, NULL, thmain4, var4);

    int *var5 = new int;
    *var5 = 5;
    pthread_create(&thid5, NULL, thmain5, var5);

    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);
    pthread_join(thid4, NULL);
    pthread_join(thid5, NULL);




    return 0;
}




void *thmain1(void *arg) {
    printf("var1 = %d.\n", *(int *)arg);
    printf("线程1开始运行.\n");
    delete (int *)arg;
    arg = NULL;
}


void *thmain2(void *arg) {
    printf("var2 = %d.\n", *(int *)arg);
    printf("线程2开始运行.\n");
    delete (int *)arg;
    arg = NULL;
}


void *thmain3(void *arg) {
    printf("var3 = %d.\n", *(int *)arg);
    printf("线程3开始运行.\n");
    delete (int *)arg;
    arg = NULL;
}


void *thmain4(void *arg) {
    printf("var4 = %d.\n", *(int *)arg);
    printf("线程4开始运行.\n");
    delete (int *)arg;
    arg = NULL;
}


void *thmain5(void *arg) {
    printf("var5 = %d.\n", *(int *)arg);
    printf("线程5开始运行.\n");
    delete (int *)arg;
    arg = NULL;
}



