/*
 *  程序名: obtcodetodb.cpp, 本程序用于把全国站点参数数据保存到数据库T_ZHOBTCODE表中
 *  作者: ZEL
 */
#include "_public.h"
#include "_mysql.h"


CLogFile      logfile;       // 日志文件操作类
connection    conn;          // 数据库连接类
CPActive      PActive;       // 进程心跳类


// 全国气象站点参数结构体
struct st_code {
    char  provname[31];     // 省
    char  obtid[11];        // 站号
    char  cityname[31];     // 站名
    char  lat[11];          // 纬度
    char  lon[11];          // 经度
    char  height[11];       // 海拔高度
};


vector<struct st_code> vstcode;  // 存放全国气象站点参数的容器


// 程序帮助文档
void _help();

// 加载全国站点参数文件到vstcode容器中
bool LoadSTCode(const char *inifile);

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
    PActive.AddPInfo(10, "obtcodetodb");

    // 把全国站点参数文件加载到vstcode容器中
    if (!LoadSTCode(argv[1])) {
        logfile.Write("LoadSTCode() failed.\n");
        EXIT(-1);
    }

    // 连接数据库
    if (conn.connecttodb(argv[2], argv[3]) != 0) {
        logfile.Write("connect database(%s) failed.\n%s\n", argv[2], conn.m_cda.message);
        EXIT(-1);
    }
    logfile.Write("connect database(%s) ok.\n", argv[2]);

    struct st_code stcode;

    // 准备插入表的SQL语句
    sqlstatement stmtins(&conn);      // SQL语句操作类
    stmtins.prepare("insert into T_ZHOBTCODE(obtid, cityname, provname, lat, lon, height, upttime) \
                  values(:1, :2, :3, :4*100, :5*100, :6*10, now())");
    stmtins.bindin(1, stcode.obtid, 10);
    stmtins.bindin(2, stcode.cityname, 30);
    stmtins.bindin(3, stcode.provname, 30);
    stmtins.bindin(4, stcode.lat, 10);
    stmtins.bindin(5, stcode.lon, 10);
    stmtins.bindin(6, stcode.height, 10);

    // 准备更新表的SQL语句
    sqlstatement stmtupt(&conn);      // SQL语句操作类
    stmtupt.prepare("update T_ZHOBTCODE set cityname = :1, provname = :2, lat = :3*100, lon = :4*100, height = :5*10, upttime = now() where obtid = :6");
    stmtupt.bindin(1, stcode.cityname, 30);
    stmtupt.bindin(2, stcode.provname, 30);
    stmtupt.bindin(3, stcode.lat, 10);
    stmtupt.bindin(4, stcode.lon, 10);
    stmtupt.bindin(5, stcode.height, 10);
    stmtupt.bindin(6, stcode.obtid, 10);

    int inscount = 0, uptcount = 0;
    CTimer  Timer;    // 计时器

    // 遍历vstcode容器
    for (int i = 0; i < vstcode.size(); i++) {
    
        // 从容器中取出一条记录到结构体stcode中
        memcpy(&stcode, &vstcode[i], sizeof(struct st_code));

        // 执行插入的SQL语句
        if (stmtins.execute() != 0) {
            // 如果记录已存在, 执行更新的SQL语句 1062
            if (stmtins.m_cda.rc == 1062) {
                if (stmtupt.execute() != 0) {
                    logfile.Write("stmtupt.execute() failed.\n%s\n%s\n", stmtupt.m_sql, stmtupt.m_cda.message);
                    EXIT(-1);
                } else uptcount++;
            } else {
                logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
                EXIT(-1);
            }
        } else inscount++;
    }

    // 把总记录数、插入记录数、更新记录数、消耗时长记录日志
    logfile.Write("总记录数 = %d, 插入 = %d, 更新 = %d, 耗时 = %.2f秒.\n", vstcode.size(), inscount, uptcount, Timer.Elapsed());

    // 提交数据库事务
    conn.commit();

    return 0;
}








void _help() {
    printf("\n");
    printf("Using:./obtcodetodb inifile connstr charset logfile\n");

    printf("Example:/project/tools/bin/procctl 120 /project/idc/bin/obtcodetodb /project/idc/ini/stcode.ini \"127.0.0.1,root,19981110,qxidc,3306\" utf8 /log/idc/obtcodetodb.log\n\n");

    printf("本程序用于把全国站点参数数据保存到数据库表中，如果站点不存在则插入，站点已存在则更新。\n");
    printf("inifile 站点参数文件名（全路径）。\n");
    printf("connstr 数据库连接参数：ip,username,password,dbname,port\n");
    printf("charset 数据库的字符集。\n");
    printf("logfile 本程序运行的日志文件名。\n");
    printf("程序每120秒运行一次，由procctl调度。\n\n\n");
}


bool LoadSTCode(const char *inifile) {

    CFile    File;    // 文件操作类
    CCmdStr  CmdStr;  // 字符串拆分类

    vstcode.clear();
    struct st_code stcode;

    // 打开文件
    if (!File.Open(inifile, "r")) {
        logfile.Write("File.Open(%s) failed.\n", inifile);
        return false;
    }

    char buffer[1024];    // 存放读取到的文件buffer

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        memset(&stcode, 0, sizeof(struct st_code));

        // 读取文件
        if (!File.Fgets(buffer, 1000, true)) break;

        // 把读取到的文件内容加载到stcode结构体
        CmdStr.SplitToCmd(buffer, ",", true);

        // 去掉无用行
        if (CmdStr.CmdCount() != 6) continue;

        // 把站点参数的每个数据项保存到站点参数结构体中
        CmdStr.GetValue(0, stcode.provname, 30);
        CmdStr.GetValue(1, stcode.obtid, 10);
        CmdStr.GetValue(2, stcode.cityname, 30);
        CmdStr.GetValue(3, stcode.lat, 10);
        CmdStr.GetValue(4, stcode.lon, 10);
        CmdStr.GetValue(5, stcode.height, 10);

        // 把站点参数结构体放入站点参数容器
        vstcode.push_back(stcode);
    }

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);

    // 断开数据库连接
    conn.disconnect();

    exit(0);
}  