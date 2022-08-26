/*
 *  程序名: migratetable.cpp, 本程序出数据中心的公共功能模块, 用于迁移表中的数据
 *  作者: ZEL
 */
#include "_public.h"
#include "_tools.h"


CLogFile       logfile;       // 日志文件操作类
connection     conn1;         // 用于执行查询SQL语句的数据库连接 
connection     conn2;         // 用于执行插入和删除SQL语句的数据库连接 
CPActive       PActive;       // 进程心跳操作类


// 程序运行的参数结构体
struct st_arg {
    char connstr[101];        // 数据库的连接参数
    char srctname[31];        // 待迁移的表名
    char dsttname[31];        // 目的表的表名
    char keycol[31];          // 待迁移的表的唯一键字段名
    char where[1001];         // 待迁移的数据需要满足的条件
    char starttime[31];       // 程序运行的时间区间
    int  maxcount;            // 每执行一次迁移操作的记录数
    int  timeout;             // 本程序运行时的超时时间
    char pname[51];           // 本程序运行时的程序名
} starg; 


// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 判断当前时间是否在程序运行的时间之内
bool instarttime();

// 清理表中数据的主函数
bool _migratetable();

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

    if (conn2.connecttodb(starg.connstr, NULL) != 0) {
        logfile.Write("connect database(%s) failed.\n%s\n", starg.connstr, conn2.m_cda.message);
        EXIT(-1);
    }

    // 清理表中数据的主函数
    _migratetable();

    return 0;
}








void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/migratetable logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/bin/migratetable /log/idc/migratetable_ZHOBTMIND.log \"<connstr>127.0.0.1,root,19981110,qxidc,3306</connstr><srctname>T_ZHOBTMIND</srctname><dsttname>T_ZHOBTMIND_HIS</dsttname><keycol>keyid</keycol><where>where ddatetime<timestampadd(minute,-120,now())</where><starttime>01,02,03,04,05,13</starttime><maxcount>300</maxcount><timeout>120</timeout><pname>migratetable_ZHOBTMIND</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于迁移表中的数据。\n");

    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("srctname    待迁移数据源表的表名\n");
    printf("dsttname    迁移目的表的表名，注意，srctname和dsttname的结构必须完全相同。\n");
    printf("keycol      待迁移数据表的唯一键字段名。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序运行时，踏中02时和13时则运行，其它时间\n"\
            "            不运行。如果starttime为空，本参数将失效，只要本程序启动就会执行数据迁移，为了减少\n"\
            "            对数据库的压力，数据迁移一般在数据库最闲的时候时进行。\n");
    printf("where       待迁移的数据需要满足的条件，即SQL语句中的where部分。\n");
    printf("maxcount    每执行一次迁移操作的记录数，不能超过MAXPARAMS(256)。\n");
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

    GetXMLBuffer(strxmlbuffer, "srctname", starg.srctname, 30);
    if (strlen(starg.srctname) == 0) {
        logfile.Write("srctname is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "dsttname", starg.dsttname, 30);
    if (strlen(starg.dsttname) == 0) {
        logfile.Write("dsttname is null.\n");
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

    GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
    if (starg.maxcount == 0) {
        logfile.Write("maxcount is null.\n");
        return false;
    }
    if (starg.maxcount > MAXPARAMS) starg.maxcount = MAXPARAMS;

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


bool _migratetable() {
    CTimer      Timer;       // 计时开始
    CTABCOLS    TABCOLS;     // 获取表全部的列和主键列信息的类

    // 从数据字典中获取表中全部的字段名
    if (!TABCOLS.allcols(&conn2, starg.dsttname)) {
        logfile.Write("表%s不存在.\n", starg.dsttname);
        return false;
    }


    char tmpvalue[51];       // 从数据源表查到的需要迁移记录的key字段的值

    // 从数据源表提取待迁移记录的唯一键, 无需排序
    sqlstatement stmtsel(&conn1);
    stmtsel.prepare("select %s from %s %s", starg.keycol, starg.srctname, starg.where);
    stmtsel.bindout(1, tmpvalue, 50);

    // 拼接绑定删除SQL语句 "where 唯一键 in (...)" 的字符串
    char strbind[2001]; memset(strbind, 0, sizeof(strbind));
    char strtemp[51];

    for (int i = 1; i <= starg.maxcount; i++) {
        memset(strtemp, 0, sizeof(strtemp));
        SNPRINTF(strtemp, 50, sizeof(strtemp), ":%lu,", i);
        strcat(strbind, strtemp);
    }

    // 删除多余的逗号
    strbind[strlen(strbind) - 1] = 0;

    char keyvalues[starg.maxcount][51];  // 存放唯一键字段的值
     
    // 准备插入和删除数据的SQL, 一次迁移starg.maxcount条记录
    sqlstatement stmtins(&conn2);
    stmtins.prepare("insert into %s(%s) select %s from %s where %s in (%s)", \
                    starg.dsttname, TABCOLS.m_allcols, TABCOLS.m_allcols, starg.srctname, starg.keycol, strbind);

    sqlstatement stmtdel(&conn2);
    stmtdel.prepare("delete from %s where %s in (%s)", starg.srctname, starg.keycol, strbind);
    
    // 绑定参数
    for (int i = 1; i < starg.maxcount; i++) {
        stmtdel.bindin(i, keyvalues[i - 1], 50);
        stmtins.bindin(i, keyvalues[i - 1], 50);
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

        // 每starg.maxcount条记录执行一次删除语句
        if (ccount == starg.maxcount) {
            // 执行插入的SQL语句
            if (stmtins.execute() != 0) {
                logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
                if (stmtins.m_cda.rc != 1062) return false;
            }

            // 执行删除的SQL语句
            if (stmtdel.execute() != 0) {
                logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
                return false;
            }

            // 提交事务
            conn2.commit();

            PActive.UptATime();
            
            ccount = 0;
            memset(keyvalues, 0, sizeof(keyvalues));
        }

    }

    // 如果不足starg.maxcount条记录, 再执行一次插入和删除
    if (ccount > 0) {
        // 执行插入的SQL语句
        if (stmtins.execute() != 0) {
            logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
            if (stmtins.m_cda.rc != 1062) return false;
        }

        // 执行删除SQL语句
        if (stmtdel.execute() != 0) {
            logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
            return false;
        }

        // 提交事务
        conn2.commit();
    }

    if (stmtdel.m_cda.rpc > 0) 
        logfile.Write("migrate %s to %s %d rows in %.02fsec.\n", starg.srctname, starg.dsttname, stmtdel.m_cda.rpc, Timer.Elapsed());

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}