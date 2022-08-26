/*
 *  程序名: procctl.cpp, 本程序是调度程序
 *  作者: ZEL
 */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>



// 程序帮助文档
void _help();







int main(int argc, char *argv[]) {
    
    // 程序帮助文档
    if (argc < 3) {
        _help();
        return -1;
    }

    // 关闭全部的IO和信号
    for (int i = 0; i < 64; i++) {
        // 关闭信号
        signal(i, SIG_IGN);

        // 关闭IO
        close(i);
    }

    // 生成子进程,父进程退出,让程序运行在后台,由1号进程托管
    if (fork() != 0) exit(0);

    // 启用SIGCHLD信号,让父进程可以wait子进程退出的状态
    signal(SIGCHLD, SIG_DFL);

    // exec是用参数中指定程序替换了当前进程的正文段,数据段,堆和栈
    // 当前程序执行到execl函数,就跑到了下一个程序,当前程序就不存在了
    char *pargv[argc];
    for (int i = 2; i < argc; i++) {
        pargv[i - 2] = argv[i];
    }

    pargv[argc - 2] = NULL;

    while (true) {
        if (fork() == 0) {
        // 子进程
        execv(argv[2], pargv);
        exit(0);
        } else {
        // 父进程
        // 等待子进程的退出
        int status;
        wait(&status);

        sleep(atoi(argv[1]));
        }
    }

    return 0;
}




void _help() {
    printf("\n");
    printf("Using: ./procctl.out timetvl program argv ...\n");
    printf("Example: /project/tools/bin/procctl 5 /usr/bin/ls -lt /home/zel\n\n");

    printf("本程序是服务程序的调度程序，周期性启动服务程序或shell脚本。\n");
    printf(
        "timetvl "
        "运行周期，单位：秒。被调度的程序运行结束后，在timetvl秒后会被procctl重新"
        "启动。\n");
    printf("program 被调度的程序名，必须使用全路径。\n");
    printf("argvs   被调度的程序的参数。\n");
    printf("注意，本程序不会被kill杀死，但可以用kill -9强行杀死。\n\n\n");
}