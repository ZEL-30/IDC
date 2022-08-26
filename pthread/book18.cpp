/*
 *  程序名: book18.cpp, 本程序演示用信号量实现高速缓存
 *  作者: ZEL
 */
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <semaphore.h>

using namespace std;


// 缓存队列消息的结构体
struct st_message {
    int  mesgid;          // 消息的ID
    char message[1024];   // 消息的内容
};

vector<struct st_message> vcache;       // 用vector容器做缓存

sem_t lock;                // 声明用于加锁的信号量
sem_t notify;              // 声明用于通知的信号量


// 生产者, 数据入队
void incache(int sig);

// 消费者, 数据出队线程主函数
void *outcache(void *arg);





int main(int argc, char const *argv[]) {

    // 初始化用于通知和加锁的信号量
    sem_init(&notify, 0, 0);           // 第三个参数填0, 是为了让消费者线程等待
    sem_init(&lock, 0, 1);             // 第三个参数填1, 表示锁可以用

    // 接收15的信号, 调用生产者函数
    signal(15, incache);

    // 创建线程
    pthread_t thid1 = 0, thid2 = 0, thid3 = 0;
    pthread_create(&thid1, NULL, outcache, NULL);
    pthread_create(&thid2, NULL, outcache, NULL);
    pthread_create(&thid3, NULL, outcache, NULL);

    // 等待线程退出
    pthread_join(thid1, NULL);
    pthread_join(thid2, NULL);
    pthread_join(thid3, NULL);

    // 销毁信号量
    sem_destroy(&notify);
    sem_destroy(&lock);


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
        
        sem_wait(&lock);          // 给缓存队列加锁
        vcache.push_back(stmesg);
        sem_post(&lock);          // 给缓存队列解锁
    }

    // 数据生产完毕, 通知线程取走数据
    // 把信号量的值加1, 将唤醒消费者线程
    sem_post(&notify);
    sem_post(&notify);
    sem_post(&notify);

}



void *outcache(void *arg) {

    struct st_message stmesg;

    while (true) {
    
        sem_wait(&lock);          // 给缓存队列加锁

        while (vcache.size() == 0) {
            // 1. 给缓存队列解锁
            sem_post(&lock);

            // 2. 阻塞等待信号量的值大于0
            sem_wait(&notify);

            // 3. 给缓存队列加锁
            sem_wait(&lock);
        }

        memset(&stmesg, 0, sizeof(struct st_message));
        memcpy(&stmesg, &vcache[0], sizeof(struct st_message));
        vcache.erase(vcache.begin());

        sem_post(&lock);          // 给缓存队列解锁

        // 以下是业务代码   
        printf("pthread = %lu, mesgid = %d, message = %s\n", pthread_self(), stmesg.mesgid, stmesg.message);
        usleep(100);
    }

    return NULL;
}
