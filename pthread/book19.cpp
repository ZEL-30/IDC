/*
 *  程序名: book19.cpp, 本程序演示用信号量实现高速缓存(多进程)
 *  作者: ZEL
 */

#include "_public.h"

CSEM lock;         // 用于给缓存队列加锁的信号量
CSEM notify;       // 用于通知的信号量

// 缓存队列消息的结构体
struct st_message {
    int  mesgid;            // 消息ID
    char message[1024];     // 消息内容
};

vector<struct st_message> vcache;    // 用vector做缓存队列




// 生产者, 数据入队
void incache(int sig);



int main(int argc, char const *argv[]) {

    lock.init(0x5000);
    notify.init(0x5001);
    notify.P();


    // 接收15的信号, 调用生产者函数
    signal(15, incache);


    // 创建进程
    int pid = fork();

    if (pid == 0) {
        struct st_message stmesg;

        while (true) {
            
            lock.P();   // 给缓存队列加锁
            if (vcache.size() == 0) {
                lock.V();   // 给缓存队列解锁
                printf("等待唤醒 ... \n");
                notify.P();
                lock.P();   // 给缓存队列加锁
            }


            memset(&stmesg, 0, sizeof(struct st_message));
            memcpy(&stmesg, &vcache[0], sizeof(struct st_message));
            vcache.erase(vcache.begin());

            lock.V();   // 给缓存队列解锁

            // 以下是业务代码
            printf("pid = %lu, mesgid = %d, message = %s\n", getpid(), stmesg.mesgid, stmesg.message);
            usleep(100);
        }
    }

    // 父进程等待子进程退出
    if (pid > 0) {
        sleep(100);
    }

    lock.destroy();
    notify.destroy();


    return 0;
}


void incache(int sig) {

    struct st_message stmesg;
    memset(&stmesg, 0, sizeof(struct st_message));
    static int mesgid = 1;

    // 生产数据
    lock.P();           // 给缓存队列加锁
    for (int i = 0; i < 5; i++) {
        stmesg.mesgid = mesgid++;
        sprintf(stmesg.message, "%d号数据内容", mesgid);

        vcache.push_back(stmesg);
    }
    lock.V();          // 给缓存队列解锁

    // 把通知信号量加1, 通知其他线程
    notify.V(5); 
}