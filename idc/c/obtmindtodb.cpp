/*
 *  程序名: obtmindtodb.cpp, 本程序用于把全国站点分钟观测数据入库到T_ZHOBTMIND表中，支持xml和csv两种文件格式
 *  作者: ZEL
 */
#include "idcapp.h"


CLogFile      logfile;       // 日志文件操作类
connection    conn;          // 数据库连接类
CPActive      PActive;       // 进程心跳类


// 程序帮助文档
void _help();

// 业务处理主函数
bool _obtmindtodb(const char *pathname, const char *connstr, const char *charset);

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
    CloseIOAndSignal();
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[4], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[4]);
        return -1;
    }

    // 把进程心跳信息写入共享内存
    PActive.AddPInfo(30, "obtmindtodb");

    // 业务处理主函数
    _obtmindtodb(argv[1], argv[2], argv[3]);

    return 0;
}







void _help() {
    printf("\n");
    printf("Using:./obtmindtodb pathname connstr charset logfile\n");

    printf("Example:/project/tools/bin/procctl 10 /project/idc/bin/obtmindtodb /idcdata/surfdata \"127.0.0.1,root,19981110,qxidc,3306\" utf8 /log/idc/obtmindtodb.log\n\n");

    printf("本程序用于把全国站点分钟观测数据保存到数据库的T_ZHOBTMIND表中，数据只插入，不更新。\n");
    printf("pathname 全国站点分钟观测数据文件存放的目录。\n");
    printf("connstr  数据库连接参数：ip,username,password,dbname,port\n");
    printf("charset  数据库的字符集。\n");
    printf("logfile  本程序运行的日志文件名。\n");
    printf("程序每10秒运行一次，由procctl调度。\n\n\n");
}


bool _obtmindtodb(const char *pathname, const char *connstr, const char *charset) {

    CDir          Dir;     // 目录操作类
    CFile         File;    // 文件操作类
    sqlstatement  stmt;    // 操作SQL语句类
    CTimer        Timer;   // 计时器, 记录每个数据文件的处理耗时
    CZHOBTMIND ZHOBTMIND(&conn, &logfile);


    int  totalcount = 0;   // 文件的总记录数
    int  insertcount = 0;  // 成功插入记录数

    bool bisxml = false;    // 文件格式: true-xml, false-csv

    // 打开目录
    if (!Dir.OpenDir(pathname, "*.xml,*.csv")) {
        logfile.Write("Dir.OpenDir(%s) failed.\n", pathname);
        return false;
    }

    while (true) {
        totalcount = insertcount = 0;

        // 读取目录, 得到一个数据文件名
        if (!Dir.ReadDir()) break;
        // 判断文件类型
        if (MatchStr(Dir.m_FullFileName, "*.xml"))
            bisxml = true;
        else  
            bisxml = false;

        // 如果数据库未连接, 连接数据库
        if (conn.m_state == 0) {
            if (conn.connecttodb(connstr, charset) != 0) {
                logfile.Write("connect database(%s) failed.\n%s\n", connstr, conn.m_cda.message);
                return false;
            }
            logfile.Write("connect database(%s) ok.\n", connstr);
        }

        // 打开文件
        if (!File.Open(Dir.m_FullFileName, "r")) {
            logfile.Write("File.Open(%s) failed.\n", Dir.m_FullFileName);
            return false;
        }

        char  strBuffer[1001];    // 存放文件中读取的一行

        while (true) {
            memset(strBuffer, 0, sizeof(strBuffer));

            // 读取文件
            if (bisxml == true) {
                if (!File.FFGETS(strBuffer, 1000, "<endl/>")) break;
            } else {
                if (!File.Fgets(strBuffer, 1000, true)) break;
                if (strstr(strBuffer, "站点") != 0) continue;
            }

            // 解析xml, 把数据解析到stobtmind结构体中
            ZHOBTMIND.SplitBuffer(strBuffer, bisxml);

            // 把结构体中的数据插入表中
            if (ZHOBTMIND.InsertTable()) 
                insertcount++;
            totalcount++;
        }   

        // 删除文件, 提交事务
        File.CloseAndRemove();
        conn.commit();

        logfile.Write("已处理文件%s(文件总记录数 = %d, 插入总数 = %d), 耗时%.2f秒.\n", Dir.m_FullFileName, totalcount, insertcount, Timer.Elapsed());
    }

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);

    // 断开数据库的连接
    conn.disconnect();

    exit(0);
}