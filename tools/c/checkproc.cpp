/*
 *  程序名: checkproc.cpp, 本程序是守护进程
 *  作者: ZEL
 */

#include "_public.h"


CLogFile   logfile;                    // 日志操作类


// 程序帮助文档
void _help();








int main(int argc, const char *argv[]) {

    // 程序帮助文档
    if (argc != 2) {
        _help();
        return -1;
    }

    // 关闭全部的信号，不希望程序被打扰
    CloseIOAndSignal(true);

    // 打开日志文件
    if (!logfile.Open(argv[1],"a+")) {
        printf("logfile.Open(%s) failed.\n", argv[1]);
        return -1;
    }

    // 创建/获取共享文件
    int shmid = 0;
    if ((shmid = shmget((key_t)SHMKEYP,MAXNUMP*sizeof(struct st_procinfo),0)) == -1) {
        printf("创建/获取共享内存(%x)失败.\n",SHMKEYP);
        return -1;
    }

    // 将共享内存连接到当前进程的地址空间
    struct st_procinfo *shm = (struct st_procinfo *)shmat(shmid,0,0);

    // 遍历共享内存中全部的进程心跳记录
    for (int i = 0;i < MAXNUMP;i++) {

        // 如果记录的pid==0, 表示空记录,continue
        if (shm[i].pid == 0) continue;

        // 如果记录的pid!=0, 表示是服务的心跳记录
        // logfile.Write("i = %d,pid = %d,pname = % s,timeout = %d,atime = %d\n",\
                      i,shm[i].pid,shm[i].pname,shm[i].timeout,shm[i].atime);

        // 向进程发送信号0, 判断它是否还存在, 如果不存在, 从共享内存中删除该记录,continue
        int iret = kill(shm[i].pid,0);
        if (iret == -1) {
            logfile.Write("进程pid=%d(%s)已经不存在.\n",shm[i].pid,shm[i].pname);
            // 从共享内存中删除该记录
            memset(&shm[i],0,sizeof(struct st_procinfo));
            continue;
        }

        // 如果进程未超时, continue
        time_t now = time(NULL);
        if (now - shm[i].atime < shm[i].timeout) continue;

        // 如果已超时
        logfile.Write("进程pid=%d(%s)已超时.\n",shm[i].pid,shm[i].pname);

        // 发送信号15, 尝试正常终止进程
        kill(shm[i].pid,15);

        // 每隔一秒判断一次进程是否存在,累计5秒,一般来说,5秒的时间足够退出
        for (int i = 0;i < 5;i++) {
            sleep(1);

            // 向进程发送信号0，判断它是否还存在
            iret = kill(shm[i].pid,0);

            // 进程已退出
            if (iret == -1) break;
        }

        // 判断进程是否仍然存在
        if (iret == -1) {
            logfile.Write("进程pid=%d(%s)已经正常退出.\n",shm[i].pid,shm[i].pname);
        } else {
            // 如果进程仍然存在, 就发送信号9, 强制终止它
            kill(shm[i].pid,9);
            logfile.Write("进程pid=%d(%s)已经强制终止.\n",shm[i].pid,shm[i].pname);
        }

        // 从共享内存中删除已超时的进程心跳记录
        memset(&shm[i],0,sizeof(struct st_procinfo));
    }

    // 把共享内存从当前的进程中分离
    shmdt(shm);

    return 0;
}








void _help() {
    printf("\n");
    printf("Using: ./checkproc logfilename\n");
    printf("Example: /project/tools/bin/procctl 10 /project/tools/bin/checkproc /tmp/log/checkproc.log\n\n");

    printf("本程序用于检查后台服务程序是否超时，如果已超时，就终止它。\n");
    printf("注意：\n");
    printf("  1）本程序由procctl启动，运行周期建议为10秒。\n");
    printf("  2）为了避免被普通用户误杀，本程序应该用root用户启动。\n");
    printf("  3）如果要停止本程序，只能用killall -9 终止。\n\n\n");
}