/*
 *  程序名: book08.cpp, 本程序演示线程的取消
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>


// 线程主函数
void *thmain(void *);





int main(int argc, char const *argv[])
{
    pthread_t thid = 0;

    // 创建线程
    pthread_create(&thid, NULL, thmain, NULL);

    sleep(1);
    pthread_cancel(thid);

    
    printf("join ... \n");
    int result = 0;
    void *ret = NULL;
    result = pthread_join(thid, &ret);
    printf("thid result = %d, ret = %d\n", result, (int)(long)ret);
    printf("join ok.\n");

    return 0;
}



void *thmain(void *) {

    // 设置线程取消状态为不可取消
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);

    for (int i = 0; i < 3; i++) {
        sleep(1);
        printf("thmain sleep(%d) ok.\n", i + 1);
    }

    return (void *)1;
}