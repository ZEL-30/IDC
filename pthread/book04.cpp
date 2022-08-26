/*
 *  程序名: book04.cpp, 本程序演示线程参数的传递(用结构体的方式传递多个参数)
 *  作者: ZEL
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

struct st_students {
    char name[20];   // 姓名
    int  age;        // 年龄
    char sid[30];    // 学号 
};

void *thmain(void *arg);


int main(int argc, char *argv[]) {

    struct st_students *st = new struct st_students;
    memset(st, 0, sizeof(struct st_students));
    strcpy(st->name, "张恩乐");
    st->age = 24;
    strcpy(st->sid, "201802464052");

    pthread_t thid;
    pthread_create(&thid, NULL, thmain, st);
    pthread_join(thid, NULL);




    return 0;
}




void *thmain(void *arg) {
    struct st_students *st = (struct st_students *)arg;
    
    printf("name = %s age = %d sid = %s\n", st->name, st->age, st->sid);

    delete (st_students *)st;
    st = NULL;
}

