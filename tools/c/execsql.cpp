/*
 *  程序名: execsql.cpp, 本程序用于执行一个sql脚本文件
 *  作者: ZEL
 */
#include "_public.h"
#include "_mysql.h"


CLogFile    logfile;       // 日志文件操作类
CPActive    PActive;       // 进程心跳操作类
connection  conn;          // 数据库连接类
CFile       File;          // 文件操作类    


// 程序帮助文档
void _help();

// 程序退出和信号2,15的处理函数
void EXIT(int sig);




int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 5) {
        _help();
        return -1;
    }

    // 关闭全部信号和输入输出
    // 设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
    // 但请不要用 "kill -9 + 进程号" 强行终止
    // CloseIOAndSignal();
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[4], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[4]);
        return -1;
    }

    // 把进程心跳写入共享内存
    PActive.AddPInfo(30, "execsql");

    // 连接数据库, 不启用事务
    if (conn.connecttodb(argv[2],argv[3], 1) != 0) {
        logfile.Write("connect database(%s) failed.\n", argv[2]);
        return -1;
    }
    logfile.Write("connect database(%s) ok.\n", argv[2]);

    // 打开文件
    if (!File.Open(argv[1], "r")) {
        logfile.Write("File.Open(%s) failed.\n", argv[1]);
        return -1;
    }

    char strsql[1024];

    while (true) {
        memset(strsql, 0, sizeof(strsql));

        // 读取文件的每一行
        if (!File.FFGETS(strsql, 1000, ";")) break;

        // 如果第一个字符是#，注释，不执行。
        if (strsql[0]=='#') continue;

        // 删除掉SQL语句最后的分号
        char *p = strstr(strsql, ";");
        if (p == 0) continue;
        p[0] = 0;

        logfile.Write("%s\n",strsql);

        // 执行SQL语句
        int iret = conn.execute(strsql);
        if (iret == 0)
            logfile.Write("exec ok(rpc = %d).\n", conn.m_cda.rpc);
        else 
            logfile.Write("exec failed(%s).\n%s\n", conn.m_cda.message);

        // 进程的心跳
        PActive.UptATime();
    }

    logfile.WriteEx("\n");

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./execsql sqlfile connstr charset logfile\n");

    printf("Example:/project/tools/bin/procctl 120 /project/tools/bin/execsql /project/idc/sql/cleardata.sql \"127.0.0.1,root,19981110,qxidc,3306\" utf8 /log/idc/execsql.log\n\n");

    printf("这是一个工具程序，用于执行一个sql脚本文件。\n");
    printf("sqlfile sql脚本文件名，每条sql语句可以多行书写，分号表示一条sql语句的结束，不支持注释。\n");
    printf("connstr 数据库连接参数：ip,username,password,dbname,port\n");
    printf("charset 数据库的字符集。\n");
    printf("logfile 本程序运行的日志文件名。\n\n");
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}