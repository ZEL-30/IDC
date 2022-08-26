/*
 *  程序名: syncupdate_oracle.cpp, 本程序是数据中心的公共功能模块，采用刷新的方法同步Oracle数据库之间的表
 *  作者: ZEL
 */
#include "_public.h"
#include "_tools_oracle.h"

#define MAXPARAMS 256
/*
1. 程序的帮助
2. 错误代码
3. keyid字段的处理
4. 时间函数和时间格式
5. oracle不需要为查询语句创建专用的数据库连接
6. oracle不需要MAXPARMS宏
7. 如果SQL语句有问题, MySQL的 `prepare()` 会返回错误, oracle的 `prepare()` 不会返回错误, 为了兼容性考虑, 尽可能不判断 `prepare()` 的返回值
8. Oracle和MySQL的数据类型的关键字不同
*/



CLogFile  logfile;               // 日志文件操作类
CPActive  PActive;               // 进程心跳操作类
connection connloc;              // 连接本地数据库
connection connrem;              // 连接远程数据库

// 程序的运行参数
struct st_arg {
  char charset[51];               // 数据库的字符集
  char localconnstr[101];         // 本地数据库的连接参数
  char lnktname[31];              // 远程表名, 在remotetname参数后加@dblink
  char localtname[31];            // 本地表名
  char remotecols[1001];          // 远程表的字段列表
  char localcols[1001];           // 本地表的字段列表
  char where[1001];               // 同步数据的条件
  int  synctype;                  // 同步方式：1-不分批同步；2-分批同步
  char remoteconnstr[101];        // 远程数据库的连接参数
  char remotetname[31];           // 远程表名
  char remotekeycol[31];          // 远程表的键值字段名
  char localkeycol[31];           // 本地表的键值字段名
  int  maxcount;                  // 每批执行一次同步操作的记录
  int  timeout;                   // 本程序运行时的超时时间
  char pname[51];                 // 本程序运行时的程序名
} starg;



// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 刷新同步Oracle主函数
bool _syncupdate();

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

    // 刷新同步Oracle的主函数
    _syncupdate(); 

    return 0;
}













void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/syncupdate_oracle logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/syncupdate_oracle /log/idc/syncupdate_oracle_ZHOBTCODE2.log \"<localconnstr>qxidc/19981110@snorcl11g_197</localconnstr><charset>Simplified Chinese_China.AL32UTF8</charset><lnktname>T_ZHOBTCODE1@db197</lnktname><localtname>T_ZHOBTCODE2</localtname><remotecols>obtid,cityname,provname,lat,lon,height,upttime,keyid</remotecols><localcols>obtid,cityname,provname,lat,lon,height,upttime,keyid</localcols><synctype>1</synctype><timeout>50</timeout><pname>syncupdate_oracle_ZHOBTCODE2</pname>\"\n\n");

    // 因为测试的需要，xmltodb程序每次会删除T_ZHOBTCODE1@db132中的数据，全部的记录重新入库，keyid会变。
    // 所以以下脚本不能用keyid，要用obtid，用keyid会出问题，可以试试。
    printf("       /project/tools/bin/procctl 10 /project/tools/bin/syncupdate_oracle /log/idc/syncupdate_oracle_ZHOBTCODE3.log \"<localconnstr>qxidc/19981110@snorcl11g_197</localconnstr><charset>Simplified Chinese_China.AL32UTF8</charset><lnktname>T_ZHOBTCODE1@db197</lnktname><localtname>T_ZHOBTCODE3</localtname><remotecols>obtid,cityname,provname,lat,lon,height,upttime,keyid</remotecols><localcols>obtid,cityname,provname,lat,lon,height,upttime,keyid</localcols><where>where obtid like '54%%%%'</where><synctype>2</synctype><remoteconnstr>qxidc/19981110@snorcl11g_197</remoteconnstr><remotetname>T_ZHOBTCODE1</remotetname><remotekeycol>obtid</remotekeycol><localkeycol>obtid</localkeycol><maxcount>10</maxcount><timeout>50</timeout><pname>syncupdate_oracle_ZHOBTCODE3</pname>\"\n\n");

    printf("       /project/tools/bin/procctl 10 /project/tools/bin/syncupdate_oracle /log/idc/syncupdate_oracle_ZHOBTMIND2.log \"<localconnstr>qxidc/19981110@snorcl11g_197</localconnstr><charset>Simplified Chinese_China.AL32UTF8</charset><lnktname>T_ZHOBTMIND1@db197</lnktname><localtname>T_ZHOBTMIND2</localtname><remotecols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</remotecols><localcols>obtid,ddatetime,t,p,u,wd,wf,r,vis,upttime,keyid</localcols><where>where ddatetime>sysdate-0.04</where><synctype>2</synctype><remoteconnstr>qxidc/19981110@snorcl11g_197</remoteconnstr><remotetname>T_ZHOBTMIND1</remotetname><remotekeycol>keyid</remotekeycol><localkeycol>keyid</localkeycol><maxcount>300</maxcount><timeout>50</timeout><pname>syncupdate_oracle_ZHOBTMIND2</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，采用刷新的方法同步Oracle数据库之间的表。\n\n");

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

    printf("where         同步数据的条件，为空则表示同步全部的记录，填充在delete本地表和select lnktname表\n"\
            "              之后，注意：1）where中的字段必须同时在本地表和lnktname表中；2）不要用系统时间作\n"\
            "              为条件，当synctype==2时无此问题。\n");

    printf("synctype      同步方式：1-不分批同步；2-分批同步。\n");
    printf("remoteconnstr 远程数据库的连接参数，格式与localconnstr相同，当synctype==2时有效。\n");
    printf("remotetname   远程表名，当synctype==2时有效。\n");
    printf("remotekeycol  远程表的键值字段名，必须是唯一的，当synctype==2时有效。\n");
    printf("localkeycol   本地表的键值字段名，必须是唯一的，当synctype==2时有效。\n");

    printf("maxcount      每批执行一次同步操作的记录数，不能超过MAXPARAMS宏，当synctype==2时有效。\n");

    printf("timeout       本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。\n");
    printf("pname         本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
    printf("注意：\n1）remotekeycol和localkeycol字段的选取很重要，如果是自增字段，那么在远程表中数据生成后自增字段的值不可改变，否则同步会失败；\n2）当远程表中存在delete操作时，无法分批同步，因为远程表的记录被delete后就找不到了，无法从本地表中执行delete操作。\n\n\n");
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

    // 远程表名, 在remotetname参数后加@dblink
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

    // 同步方式：1-不分批同步；2-分批同步。
    GetXMLBuffer(strxmlbuffer, "synctype", &starg.synctype);
    if ( (starg.synctype != 1) && (starg.synctype != 2) ) { 
        logfile.Write("synctype is not in (1,2).\n"); 
        return false; 
    }

    if (starg.synctype == 2)
    {
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

    // 远程表的键值字段名，必须是唯一的，当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "remotekeycol", starg.remotekeycol, 30);
    if (strlen(starg.remotekeycol) == 0) { 
        logfile.Write("remotekeycol is null.\n"); 
        return false; 
    }

    // 本地表的键值字段名，必须是唯一的，当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "localkeycol", starg.localkeycol, 30);
    if (strlen(starg.localkeycol) == 0) { 
        logfile.Write("localkeycol is null.\n"); 
        return false; 
    }

    // 每批执行一次同步操作的记录数，不能超过MAXPARAMS宏（在_Oracle.h中定义），当synctype==2时有效。
    GetXMLBuffer(strxmlbuffer, "maxcount", &starg.maxcount);
    if (starg.maxcount == 0) { 
        logfile.Write("maxcount is null.\n"); 
        return false; 
    }
    if (starg.maxcount > MAXPARAMS) 
        starg.maxcount = MAXPARAMS;
    }

    // 本程序的超时时间，单位：秒，视数据量的大小而定，建议设置30以上。
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout == 0) { 
        logfile.Write("timeout is null.\n"); 
        return false; 
    }

    // 本程序运行时的进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。
    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
    if (strlen(starg.pname) == 0) { 
        logfile.Write("pname is null.\n"); 
        return false; 
    }

    return true;
}


bool _syncupdate() {

    sqlstatement  stmtdel(&connloc);           // 执行删除本地表中记录的SQL语句
    sqlstatement  stmtins(&connloc);           // 执行向本地表中插入数据的SQL语句
    CTimer        Timer;                       // 开始计时


    // 如果是不分批同步, 表示需要同步的数据量比较少, 执行一次SQL语句就可以搞定
    if (starg.synctype == 1) {

        logfile.Write("sync %s to %s ... ", starg.lnktname, starg.localtname);

        // 先删除starg.localtname表中满足where条件的记录
        stmtdel.prepare("delete from %s %s", starg.localtname, starg.where);

        // 执行删除的SQL语句
        if (stmtdel.execute() != 0) {
            logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
            return false;
        }

        // 再把starg.lnktname表中满足where条件的记录插入到starg.localtname表中
        stmtins.prepare("insert into %s(%s) select %s from %s %s", starg.localtname, starg.localcols, starg.remotecols, starg.lnktname, starg.where);

        // 执行插入的SQL语句
        if (stmtins.execute() != 0) {
            logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);

            // 回滚事务
            connloc.rollback();

            return false;
        }

        logfile.WriteEx("%d rows in %.02fsec.\n", stmtins.m_cda.rpc, Timer.Elapsed());

        // 提交事务
        connloc.commit();
        return true;
    }

    // 连接远程数据库
    if (connrem.connecttodb(starg.remoteconnstr, starg.charset) != 0) {
        logfile.Write("connect database(%s) failed.\n", starg.remoteconnstr);
        return false;
    }

    // 从远程表查找需要同步记录的key字段的值
    sqlstatement stmtsel(&connrem);
    char remkeyvalue[51];  // 从远程表查到的需要同步记录的key字段的值
    stmtsel.prepare("select %s from %s %s", starg.remotekeycol,
                    starg.remotetname, starg.where);
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
    
    // 准备删除本地表数据的SQL语句, 一次删除starg.maxcount条记录
    // delete from T_ZHOBTCODE3 where stid in (:1,:2,:3,...,:starg.maxcount);
    stmtdel.prepare("delete from %s where %s in (%s)", starg.localtname, starg.localkeycol, strbind);
    for (int i = 1; i <= starg.maxcount; i++) {
        stmtdel.bindin(i, keyvalues[i - 1], 50);
    }

    // 准备插入本地表数据的SQL语句, 一次删除starg.maxcount条记录
    // insert into T_ZHOBTCODE3(stid ,cityname,provname,lat,lon,altitude,upttime,keyid)
    //                   select obtid,cityname,provname,lat,lon,height/10,upttime,keyid from LK_ZHOBTCODE1 
    //                    where obtid in (:1,:2,:3);
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

            // 从本地表中删除记录
            if (stmtdel.execute() != 0) {
                logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
                return false;
            }

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

    // 如果够一批
    if (ccount > 0) {

        // 从本地表中删除记录
        if (stmtdel.execute() != 0) {
            logfile.Write("stmtdel.execute() failed.\n%s\n%s\n", stmtdel.m_sql, stmtdel.m_cda.message);
            return false;
        }

        // 向本地表中插入记录
        if (stmtins.execute() != 0) {
            logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);
            return false;
        }

        logfile.Write("sync %s to %s(%d rows) in %.02fsec.\n", starg.lnktname, starg.localtname, ccount, Timer.Elapsed());

        // 提交事务
        connloc.commit();
    }

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}            