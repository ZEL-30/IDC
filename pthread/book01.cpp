/*
 *  程序名: book1.cpp, 本程序演示线程的创建
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


// 线程主函数
void *thmain(void *); 

int main(int argc, char *argv[]) {

    // 创建线程ID
    pthread_t thid = 0;

    // 创建线程
    if (pthread_create(&thid, NULL, thmain, NULL) != 0) {
       printf("pthread_create() failed.\n");
       return -1; 
    }

    // 等待线程主函数退出
    printf("join ... \n");
    pthread_join(thid, NULL);
    printf("join ok.\n");


    return 0;
}


void *thmain(void *) {

    for (int i = 0; i < 500; i++) {
        sleep(1);
        printf("pthmian sleep(%d) ok.\n", i + 1);

    }

    return 0;
}
