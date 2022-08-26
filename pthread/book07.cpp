/*
 *  程序名: book07.cpp, 本程序演示线程资源的回收（线程清理函数）
 *  作者: zel
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>



// 线程主函数
void *thmain(void *arg);  

// 线程清理函数
void thcleanup1(void *arg);
void thcleanup2(void *arg);
void thcleanup3(void *arg);




int main(int argc, char *argv[]) {
    pthread_t thid;

    // 创建线程。
    if (pthread_create(&thid, NULL, thmain, NULL) != 0) {
        printf("pthread_create failed.\n");
        exit(-1);
    }

    int result = 0;
    void *ret;
    printf("join...\n");
    result = pthread_join(thid, &ret);
    printf("thid result=%d,ret=%ld\n", result, ret);
    printf("join ok.\n");
}



void *thmain(void *arg) {
/*
    // 线程主函数入栈, 关闭文件指针
    pthread_cleanup_push(thcleanup1, fd);
    
    // 线程主函数入栈, 关闭socket
    pthread_cleanup_push(thcleanup2, socket);
    
    // 线程主函数入栈, 回滚数据库事务
    pthread_cleanup_push(thcleanup3, &conn);
*/

    for (int ii = 0; ii < 3; ii++) {
        sleep(1);
        printf("pthmain sleep(%d) ok.\n", ii + 1);
    }

/*
    // 线程主函数出栈
    pthread_cleanup_pop(3);
    pthread_cleanup_pop(2);
    pthread_cleanup_pop(1);
*/
}


void thcleanup1(void *) {

    printf("thcleanup1 cleanup ok.\n");
}


void thcleanup2(void *) {

    printf("thcleanup2 cleanup ok.\n");
}


void thcleanup3(void *) {

    printf("thcleanup3 cleanup ok.\n");
}