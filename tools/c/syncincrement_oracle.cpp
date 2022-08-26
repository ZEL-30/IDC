/*
 *  程序名: syncincrement_oracle.cpp, 本程序是数据中心的公共功能模块，采用增量的方法同步Oracle数据库之间的表
 *  作者: ZEL
 */
#include "_public.h"
#include "_tools_oracle.h"

#define MAXPARAMS 256

CLogFile  logfile;                // 日志文件操作类
CPActive  PActive;                // 进程心跳操作类
connection connloc;               // 连接本地数据库
connection connrem;               // 连接远程数据库


// 程序的运行参数
struct st_arg {
    char localconnstr[101];       // 本地数据库的连接参数
    char charset[51];             // 数据库的字符集
    char lnktname[31];            // 远程表名，在remotetname参数后加@dblink
    char localtname[31];          // 本地表名
    char remotecols[1001];        // 远程表的字段列表
    char localcols[1001];         // 本地表的字段列表
    char where[1001];             // 同步数据的条件
    char remoteconnstr[101];      // 远程数据库的连接参数
    char remotetname[31];         // 远程表名
    char remotekeycol[31];        // 远程表的自增字段名
    char localkeycol[31];         // 本地表的自增字段名
    int  maxcount;                // 每批执行一次同步操作的记录数
    int  timetvl;                 // 同步时间间隔，单位：秒，取值1-30
    int  timeout;                 // 本程序运行时的超时时间
    char pname[51];               // 本程序运行时的程序名
} starg;


long maxkeyvalue = 0;

// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 增量同步Oracle主函数
bool _syncincrement(bool &bcontinue);

//  从本地表starg.localtname获取自等字段的最大值, 存放在maxkeyvalue全局变量中 
bool findmaxkey();

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

    // 解析XML, 获取程序运行参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg(%s) failed.\n", argv[2]);
        return -1;
    }

    // 连接本地数据库
    if (connloc.connecttodb(starg.localconnstr, starg.charset) != 0) {
        logfile.Write("connect databese(%s) failed.\n", starg.localconnstr);
        return -1;
    }
    logfile.Write("connect to database(%s) ok.\n", starg.localconnstr);

    // 连接远程数据库
    if (connrem.connecttodb(starg.remoteconnstr, starg.charset) != 0) {
        logfile.Write("connect database(%s) failed.\n", starg.remoteconnstr);
        return false;
    }
    logfile.Write("connect to database(%s) ok.\n", starg.remoteconnstr);

    // 如果starg.remotecols或starg.localcols为空, 就用starg.localtname表的全部列来充填
    if (strlen(starg.remotecols) == 0 || strlen(starg.localcols) == 0) {

        CTABCOLS   TABCOLS;
        if (!TABCOLS.allcols(&connloc, starg.localtname)) {
            logfile.Write("表(%s)不存在.\n", starg.localtname);
            EXIT(-1);
        } 

        if (strlen(starg.remotecols) == 0) 
            strcpy(starg.remotecols, TABCOLS.m_allcols);

        if (strlen(starg.localcols) == 0) 
            strcpy(starg.localcols, TABCOLS.m_allcols);
    }

    bool bcontinue;

    while (true) {
        // 刷新同步Oracle的主函数
        if (!_syncincrement(bcontinue)) EXIT(-1); 

        if (bcontinue == false) sleep(starg.timeout);

        // 更新进程心跳时间
        PActive.UptATime();
    }

    return 0;
}













void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/syncincrement_oracle logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/syncincrement_oracle /log/idc/syncincrement_oracle_ZHOBTMIND2.log \"<localconnstr>qxidc/19981110@snorcl11g_197</localconnstr><remoteconnstr>qxidc/19981110@snorcl11g_197</remoteconnstr><charset>Simplified Chinese_China.AL32UTF8</charset><remotetname>T_ZHOBTMIND1</remotetname><lnktname>T_ZHOBTMIND1@db197</lnktname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</localcols><remotekeycol>keyid</remotekeycol><localkeycol>keyid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement_oracle_ZHOBTMIND2</pname>\"\n\n");

    printf("       /project/tools/bin/procctl 10 /project/tools/bin/syncincrement_oracle /log/idc/syncincrement_oracle_ZHOBTMIND3.log \"<localconnstr>qxidc/19981110@snorcl11g_197</localconnstr><remoteconnstr>qxidc/19981110@snorcl11g_197</remoteconnstr><charset>Simplified Chinese_China.AL32UTF8</charset><remotetname>T_ZHOBTMIND1</remotetname><lnktname>T_ZHOBTMIND1@db197</lnktname><localtname>T_ZHOBTMIND3</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</localcols><where>and obtid like '54%%%%'</where><remotekeycol>keyid</remotekeycol><localkeycol>keyid</localkeycol><maxcount>300</maxcount><timetvl>2</timetvl><timeout>50</timeout><pname>syncincrement_oracle_ZHOBTMIND3</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，采用增量的方法同步Oracle数据库之间的表。\n\n");

    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("localconnstr  本地数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("charset       数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。\n");

    printf("lnktname      远程表名，在remotetname参数后加@dblink。\n");
    printf("localtname    本地表名。\n");

    printf("remotecols    远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，\n"\
            "              也可以是函数的返回值或者运算结果。如果本参数为空，就用localtname表的字段列表填充。\n");
    printf("localcols     本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，\n"\
            "              就用localtname表的字段列表填充。\n");

    printf("where         同步数据的条件，填充在select remotekeycol from remotetname where remotekeycol>:1之后，\n"\
            "              注意，不要加where关键字，但是，需要加and关键字。\n");

    printf("remoteconnstr 远程数据库的连接参数，格式与localconnstr相同。\n");
    printf("remotetname   远程表名。\n");
    printf("remotekeycol  远程表的自增字段名。\n");
    printf("localkeycol   本地表的自增字段名。\n");

    printf("maxcount      每批执行一次同步操作的记录数，不能超过MAXPARAMS宏，当synctype==2时有效。\n");

    printf("timetvl       执行同步的时间间隔，单位：秒，取值1-30。\n");
    printf("timeout       本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
    printf("pname         本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {
    memset(&starg, 0, sizeof(struct st_arg));

    // 本地数据库的连接参数，格式：ip,username,password,dbname,port。
    GetXMLBuffer(strxmlbuffer, "localconnstr", starg.localconnstr, 100);
    if (strlen(starg.localconnstr) == 0) {
        logfile.Write("localconnstr is null.\n");
        return false;
    }

    // 数据库的字符集，这个参数要与远程数据库保持一致，否则会出现中文乱码的情况。
    GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
    if (strlen(starg.charset) == 0) {
        logfile.Write("charset is null.\n");
        return false;
    }

    // Federated表名。
    GetXMLBuffer(strxmlbuffer, "lnktname", starg.lnktname, 30);
    if (strlen(starg.lnktname) == 0) {
        logfile.Write("lnktname is null.\n");
        return false;
    }

    // 本地表名。
    GetXMLBuffer(strxmlbuffer, "localtname", starg.localtname, 30);
    if (strlen(starg.localtname) == 0) {
        logfile.Write("localtname is null.\n");
        return false;
    }

    // 远程表的字段列表，用于填充在select和from之间，所以，remotecols可以是真实的字段，也可以是函数
    // 的返回值或者运算结果。如果本参数为空，就用localtname表的字段列表填充。\n");
    GetXMLBuffer(strxmlbuffer, "remotecols", starg.remotecols, 1000);

    // 本地表的字段列表，与remotecols不同，它必须是真实存在的字段。如果本参数为空，就用localtname表的字段列表填充。
    GetXMLBuffer(strxmlbuffer, "localcols", starg.localcols, 1000);

    // 同步数据的条件，即select语句的where部分。
    GetXMLBuffer(strxmlbuffer, "where", starg.where, 1000);

    // 远程数据库的连接参数，格式与localconnstr相同，当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "remoteconnstr", starg.remoteconnstr, 100);
    if (strlen(starg.remoteconnstr) == 0) {
        logfile.Write("remoteconnstr is null.\n");
        return false;
    }

    // 远程表名，当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "remotetname", starg.remotetname, 30);
    if (strlen(starg.remotetname) == 0) {
        logfile.Write("remotetname is null.\n");
        return false;
    }

    // 远程表的自增字段名。
    GetXMLBuffer(strxmlbuffer, "remotekeycol", starg.remotekeycol, 30);
    if (strlen(starg.remotekeycol) == 0) {
        logfile.Write("remotekeycol is null.\n");
        return false;
    }

    // 本地表的自增字段名。
    GetXMLBuffer(strxmlbuffer, "localkeycol", starg.localkeycol, 30);
    if (strlen(starg.localkeycol) == 0) {
        logfile.Write("localkeycol is null.\n");
        return false;
    }

    // 每批执行一次同步操作的记录数，不能超过MAXPARAMS宏（在_mysql.h中定义），当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
    if (starg.maxcount == 0) {
        logfile.Write("maxcount is null.\n");
        return false;
    }
    if (starg.maxcount > MAXPARAMS) starg.maxcount = MAXPARAMS;

    // 执行同步的时间间隔，单位：秒，取值1-30。
    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
    if (starg.timetvl <= 0) {
        logfile.Write("timetvl is null.\n");
        return false;
    }
    if (starg.timetvl > 30) starg.timetvl = 30;

    // 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout == 0) {
        logfile.Write("timeout is null.\n");
        return false;
    }

    // 以下处理timetvl和timeout的方法虽然有点随意，但也问题不大，不让程序超时就可以了。
    if (starg.timeout < starg.timetvl + 10) starg.timeout = starg.timetvl + 10;

    // 本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。
    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
    if (strlen(starg.pname) == 0) {
        logfile.Write("pname is null.\n");
        return false;
    }

  return true;
}


bool _syncincrement(bool &bcontinue) {

    CTimer        Timer;                       // 开始计时

    bcontinue = false;

    // 从本地表starg.localtname获取自增字段的最大值, 存放在maxkeyvalue全局变量中
    if (!findmaxkey()) return false;

    // 从远程表查找自增字段的值大于maxkeyvalue的记录 
    sqlstatement stmtsel(&connrem);
    char remkeyvalue[51];  // 从远程表查到的需要同步记录的key字段的值
    stmtsel.prepare("select %s from %s where %s > :1 %s order by %s", starg.remotekeycol, starg.remotetname, starg.remotekeycol, starg.where, starg.remotekeycol);
    stmtsel.bindin(1, &maxkeyvalue);
    stmtsel.bindout(1, remkeyvalue, 50);

    // 拼接绑定同步SQL语句参数的字符串（:1,:2,:3,...,:starg.maxcount）
    char strbind[2001];           // 绑定同步SQL参数的字符串
    char strtemp[11];

    memset(strbind, 0, sizeof(strbind));
    for (int i = 1; i <= starg.maxcount; i++) {
        memset(strtemp, 0, sizeof(strtemp));
        SNPRINTF(strtemp, 10, sizeof(strtemp), ":%lu,", i);
        strcat(strbind, strtemp);
    }

    // 删除多余的逗号
    strbind[strlen(strbind) - 1] = 0;

    char keyvalues[starg.maxcount][51];        // 存放key字段的值
    

    // 准备插入本地表数据的SQL语句, 一次删除starg.maxcount条记录
    sqlstatement  stmtins(&connloc);           // 执行向本地表中插入数据的SQL语句
    stmtins.prepare("insert into %s(%s) select %s from %s where %s in (%s)", \
                    starg.localtname, starg.localcols, starg.remotecols, starg.lnktname, starg.remotekeycol, strbind);
    for (int i = 1; i <= starg.maxcount; i++) {
        stmtins.bindin(i, keyvalues[i - 1], 50);
    } 

    int ccount = 0;    // 记录从结果集中已获取记录的计数器
    memset(keyvalues, 0, sizeof(keyvalues));

    // 执行查询的SQL语句
    if (stmtsel.execute() != 0) {
        logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
        return false;
    }

    while (true) {

        // 获取需要同步数据的结果集
        if (stmtsel.next() != 0) break;

        strcpy(keyvalues[ccount], remkeyvalue);

        ccount++;

        // 如果够一批
        if (ccount == starg.maxcount) {

            // 向本地表中插入记录
            if (stmtins.execute() != 0) {
                logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
                return false;
            }

            logfile.Write("sync %s to %s(%d rows) in %.02fsec.\n", starg.lnktname, starg.localtname, ccount, Timer.Elapsed());

            // 提交事务
            connloc.commit();

            ccount=0;    // 记录从结果集中已获取记录的计数器
            memset(keyvalues, 0, sizeof(keyvalues));

            PActive.UptATime(); 
        }
    }

    // 如果不够一批
    if (ccount > 0) {

        // 向本地表中插入记录
        if (stmtins.execute() != 0) {
            logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
            return false;
        }

        logfile.Write("sync %s to %s(%d rows) in %.02fsec.\n", starg.lnktname, starg.localtname, ccount, Timer.Elapsed());

        // 提交事务
        connloc.commit();
    }

    if (stmtsel.m_cda.rpc > 0) bcontinue = true;


    return true;
}


bool findmaxkey() {
    maxkeyvalue = 0;

    sqlstatement  stmtsel(&connloc);
    stmtsel.prepare("select max(%s) from %s", starg.localkeycol, starg.localtname);
    // 绑定输出参数
    stmtsel.bindout(1, &maxkeyvalue);

    // 执行查询的SQL语句
    if (stmtsel.execute() != 0) {
        logfile.Write("stmtsel.execute() failed.\n%s\n%s\n", stmtsel.m_sql, stmtsel.m_cda.message);
        return false;
    }

    // 获取结果集
    stmtsel.next();

    return true;
}

void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}            