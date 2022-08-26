/*
 *  程序名: deletetable.cpp, 本程序出数据中心的公共功能模块, 用于定时清理表中的数据
 *  作者: ZEL
 */
#include "_public.h"
#include "_mysql.h"


CLogFile       logfile;       // 日志文件操作类
connection     conn1;         // 用于执行查询SQL语句的数据库连接 
connection     conn2;         // 用于执行删除SQL语句的数据库连接 
CPActive       PActive;       // 进程心跳操作类


// 程序运行的参数结构体
struct st_arg {
    char connstr[101];     // 数据库的连接参数
    char tname[31];        // 待清理的表名
    char keycol[31];       // 待清理的表的唯一键字的名
    char where[1001];      // 待清理的数据需要满足的条件
    char starttime[31];    // 程序运行的时间区间
    int  timeout;          // 本程序运行时的超时时间
    char pname[51];        // 本程序运行时的程序名
} starg;


// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 判断当前时间是否在程序运行的时间之内
bool instarttime();

// 清理表中数据的主函数
bool _deletetable();

// 程序退出和信号2,15的处理函数
void EXIT(int sig);




int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    // 关闭全部信号和输入输出
    // 设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
    // 但请不要用 "kill -9 + 进程号" 强行终止
    // CloseIOAndSignal();
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[1]);
        return -1;
    }

    // 把进程的心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    // 判断当前时间是否在程序运行的时间之内
    if (!instarttime()) return 0;

    // 解析XML, 获得程序运行参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg(%s) failed.\n", argv[2]);
        return -1;
    }

    // 连接数据库
    if (conn1.connecttodb(starg.connstr, NULL) != 0) {
        logfile.Write("connect database(%s) failed.\n%s\n", starg.connstr, conn1.m_cda.message);
        EXIT(-1);
    }

    // 注意第三个参数, 开启自动提交
    if (conn2.connecttodb(starg.connstr, NULL, 1) != 0) {
        logfile.Write("connect database(%s) failed.\n%s\n", starg.connstr, conn2.m_cda.message);
        EXIT(-1);
    }

    // 清理表中数据的主函数
    _deletetable();

    return 0;
}








void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/deletetable logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/bin/deletetable /log/idc/deletetable_ZHOBTMIND1.log \"<connstr>127.0.0.1,root,19981110,qxidc,3306</connstr><tname>T_ZHOBTMIND1</tname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><timeout>120</timeout><pname>deletetable_ZHOBTMIND1</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于定时清理表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("tname       待清理数据表的表名。\n");
    printf("keycol      待清理数据表的唯一键字段名。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间\n"\
            "            不运行。如果starttime为空，本参数将失效，只要本程序启动就会执行数据迁移，为了减少\n"\
            "            对数据库的压力，数据迁移一般在数据库最闲的时候时进行。\n");
    printf("where       待清理的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("timeout     本程序的超时时间，单位：秒，建议设置120以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {
    memset(&starg, 0, sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
    if (strlen(starg.connstr) == 0) {
        logfile.Write("connstr is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "tname", starg.tname, 30);
    if (strlen(starg.tname) == 0) {
        logfile.Write("tname is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "keycol", starg.keycol, 30);
    if (strlen(starg.keycol) == 0) {
        logfile.Write("keycol is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);
    if (strlen(starg.where) == 0) {
        logfile.Write("where is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 30);

    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout == 0) {
        logfile.Write("timeout is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
    if (strlen(starg.pname) == 0) {
        logfile.Write("pname is null.\n");
        return false;
    }

    return true;
}


bool instarttime() {

    // 获取系统当前时间-小时
    char nowhour[11];  memset(nowhour, 0, sizeof(nowhour));
    LocalTime(nowhour, "hh24");

    // 判断starg.starttime是否为空
    if (strlen(starg.starttime) == 0) return true;

    // 判断是否踏中程序运行区间
    if (strstr(starg.starttime, nowhour) == NULL)
        return false;

    return true;
}


bool _deletetable() {
    CTimer   Timer;   // 计时开始

    char tmpvalue[51];       // 存放从表提取待删除记录的唯一键的值

    // 从表提取待删除记录的唯一键 
    sqlstatement stmtsel(&conn1);
    stmtsel.prepare("select %s from %s %s", starg.keycol, starg.tname, starg.where);
    stmtsel.bindout(1, tmpvalue, 50);

    // 拼接绑定删除SQL语句 "where 唯一键 in (...)" 的字符串
    char strbind[2001]; memset(strbind, 0, sizeof(strbind));
    char strtemp[51];

    for (int i = 1; i <= MAXPARAMS; i++) {
        memset(strtemp, 0, sizeof(strtemp));
        SNPRINTF(strtemp, 50, sizeof(strtemp), ":%lu,", i);
        strcat(strbind, strtemp);
    }

    // 删除多余的逗号
    strbind[strlen(strbind) - 1] = 0;

    char keyvalues[MAXPARAMS][51];  // 存放唯一键字段的值
     
    // 准备删除数据的SQL, 一次删除MAXPARAMS条记录
    sqlstatement stmtdel(&conn2);
    stmtdel.prepare("delete from %s where %s in (%s)", starg.tname, starg.keycol, strbind);
    
    // 绑定参数
    for (int i = 1; i <= MAXPARAMS; i++) {
        stmtdel.bindin(i, keyvalues[i - 1], 50);
    }

    int ccount = 0;
    memset(keyvalues, 0, sizeof(keyvalues));

    // 执行查询的SQL语句
    if (stmtsel.execute() != 0) {
        logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
        return false;
    }

    while (true) {

        memset(tmpvalue, 0, sizeof(tmpvalue));

        // 获取结果集
        if (stmtsel.next() != 0) break;

        // 把临时变量tmpvalue保存到keyvalues数组中
        strcpy(keyvalues[ccount], tmpvalue);

        ccount++;

        // 每MAXPARAMS条记录执行一次删除语句
        if (ccount == MAXPARAMS) {
            // 执行删除SQL语句
            if (stmtdel.execute() != 0) {
                logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
                return false;
            }

            PActive.UptATime();
            
            ccount = 0;
            memset(keyvalues, 0, sizeof(keyvalues));
        }

    }

    // 如果不足MAXPARAMS条记录, 再执行一次删除
    if (ccount > 0) {
        // 执行删除SQL语句
        if (stmtdel.execute() != 0) {
            logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
            return false;
        }

    }

    if (stmtdel.m_cda.rpc > 0) 
        logfile.Write("delete from %s %d rows in %.02fsec.\n", starg.tname, stmtdel.m_cda.rpc, Timer.Elapsed());

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}