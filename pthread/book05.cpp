/*
 *  程序名: book05.cpp, 本程序演示线程退出(终止)的状态
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

void *thmain(void *arg);

struct st_ret {
    int  recode;           // 返回代码
    char message[1024];    // 返回内容
};

int main(int argc, char *argv[]) {

    pthread_t thid;
    pthread_create(&thid, NULL, thmain, NULL);

    void *pv = NULL;
    pthread_join(thid, &pv);

    struct st_ret *ret = (struct st_ret *)pv;
    printf("recode = %d message = %s\n", ret->recode, ret->message);

    delete ret; ret = NULL;
    return 0;
}




void *thmain(void *arg) {
    printf("线程%lu开始运行.\n", pthread_self());

    struct st_ret *ret = new struct st_ret;
    ret->recode = 0;
    strcpy(ret->message, "程序运行情况良好");


    pthread_exit((void *)ret);
}

