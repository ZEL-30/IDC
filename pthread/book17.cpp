/*
 *  程序名: book17.cpp, 本程序演示用互斥锁 + 条件变量实现高速缓存
 *  作者: ZEL
 */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

using namespace std;


// 缓存队列消息的结构体
struct st_message {
    int  mesgid;          // 消息的ID
    char message[1024];   // 消息的内容
};

vector<struct st_message> vcache;       // 用vector容器做缓存

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;   // 声明并初始化互斥锁
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;      // 声明并初始化条件变量


// 生产者, 数据入队
void incache(int sig);

// 消费者, 数据出队线程主函数
void *outcache(void *arg);

// 线程清理函数
void pthcleanup(void *arg);





int main(int argc, char const *argv[]) {
    
    // 接收15的信号, 调用生产者函数
    signal(15, incache);

    // 创建线程
    pthread_t thid1 = 0, thid2 = 0, thid3 = 0;
    pthread_create(&thid1, NULL, outcache, NULL);
    pthread_create(&thid2, NULL, outcache, NULL);
    pthread_create(&thid3, NULL, outcache, NULL);

    // 取消全部线程
    sleep(2);
    pthread_cancel(thid1);
    pthread_cancel(thid2);
    pthread_cancel(thid3);

    // 等待线程退出
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);

    // 销毁锁
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);


    return 0;
}




void incache(int sig) {

    struct st_message stmesg;
    memset(&stmesg, 0, sizeof(struct st_message));

    // 生产数据
    static int mesgid = 1;
    for (int i = 0; i < 5; i++) {
        stmesg.mesgid = mesgid++;
        sprintf(stmesg.message, "%d号数据", mesgid);
        
        pthread_mutex_lock(&mutex);          // 加锁
        vcache.push_back(stmesg);
        pthread_mutex_unlock(&mutex);        // 解锁
    }

    // 数据生产完毕, 通知线程取走数据
    pthread_cond_broadcast(&cond);

}



void *outcache(void *arg) {

    // 注册清理函数
    pthread_cleanup_push(pthcleanup, NULL);


    struct st_message stmesg;

    while (true) {
    
        pthread_mutex_lock(&mutex);          // 加锁

        while (vcache.size() == 0) {
            pthread_cond_wait(&cond, &mutex);
        }

        memset(&stmesg, 0, sizeof(struct st_message));
        memcpy(&stmesg, &vcache[0], sizeof(struct st_message));
        vcache.erase(vcache.begin());

        pthread_mutex_unlock(&mutex);        // 解锁

        // 以下是业务代码   
        printf("pthread = %lu, mesgid = %d, message = %s\n", pthread_self(), stmesg.mesgid, stmesg.message);
        usleep(100);
    }


    // 清理函数出栈
    pthread_cleanup_pop(1);

    return NULL;
}


void pthcleanup(void *arg) {

    // 在这里释放关闭文件、断开网络连接、回滚数据库事务、释放锁等等
    printf("cleanup ok.\n");

    // 一定记得要把互斥锁解锁
    pthread_mutex_unlock(&mutex);
}