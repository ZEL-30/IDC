/*
 *  程序名: dminingmysql.cpp, 本程序是数据中心的公共功能模块, 用于从mysql数据库源表抽取数据, 生成xml文件
 *  作者: ZEL
 */
#include "_public.h"
#include "_mysql.h"


#define MAXFIELDCOUNT 100    // 结果集字段的最大值


CLogFile    logfile;           // 日志文件操作类
CPActive    PActive;           // 进程心跳类
connection  conn, conn1;       // 数据库连接类


// 程序运行的参数结构体
struct st_arg {
    char connstr[101];      // 数据库的连接参数
    char charset[51];       // 数据库的字符集
    char selectsql[1024];   // 从数据源数据库抽取数据的SQL语句
    char fieldstr[501];     // 抽取数据的SQL语句输出结果集字段名, 字段名之间用逗号分隔
    char fieldlen[501];     // 抽取数据的SQL语句输出结果集字段的长度, 用逗号分隔
    char bfilename[31];     // 输出xml文件的前缀
    char efilename[31];     // 输出xml文件的后缀
    char outpath[301];      // 输出xml文件存放的目录
    int  maxcount;          // 输出xml文件最大记录数, 0表示无限制
    char starttime[52];     // 程序运行的时间区间
    char incfield[31];      // 递增字段名
    char incfilename[301];  // 已抽取数据的递增字段最大值存放的文件
    char connstr1[101];     // 已抽取数据的递增字段最大值存放的数据库的连接参数
    int  timeout;           // 进程心跳的超时时间
    char pname[51];         // 进程名, 建议用 “dminingmysql_后缀” 的方式
} starg;


char strfieldname[MAXFIELDCOUNT][31];   // 结果集字段名数组, 从starg.fieldstr解析得到
char strxmlfilename[301];               // xml文件名
long imaxincvalue;                      // 自增字段的最大值
int  MAXFIELDLEN = -1;                  // 结果集字段值的最大长度，存放fieldlen数组中元素的最大值       
int  ifieldlen[MAXFIELDCOUNT];          // 结果集字段的长度数组, 从starg.fieldlen解析得到
int  ifieldcount;                       // strfiledname和ifieldlen数组中的有效字段的个数
int  incfieldpos = -1;                  // 递增字段在结果集数组中的位置


// 程序帮助文档 
void _help();

// 解析xml,得到程序运行的参数
bool _xmltoarg(const char *strxmlbuffer);

// 判断当前时间是否在程序运行时间内
bool inStartTime();

// 数据抽取主函数
bool _dminingmysql();

// 从数据表中或incfile文件中获取已抽取数据的最大id
bool readIncField();

// 生成XML文件名
void crtXmlFileName();

// 把已抽取数据的最大id写入数据表或starg.incfilename文件中
bool writeIncField();

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
    CloseIOAndSignal();
    signal(SIGINT, EXIT); signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[1]);
        return -1;
    }

    // 解析xml, 获得程序参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg() failed.\n", argv[2]);
        return -1;
    }

    // 判断当前时间是否在程序运行的时间区间内
    if (!inStartTime()) return false;

    // 把进程心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    // 连接数据源的数据库
    if (conn.connecttodb(starg.connstr, starg.charset) != 0) {
        logfile.Write("connect database(%s) failed.\n", starg.connstr);
        return -1; 
    }
    logfile.Write("connect database(%s) ok.\n", starg.connstr);

    // 连接本地数据库, 用于存放已抽取数据的自增字段的最大值
    if (strlen(starg.connstr1) != 0) {
        if (conn1.connecttodb(starg.connstr1, starg.charset) != 0) {
            logfile.Write("connect database(%s) failed.\n", starg.connstr1);
            return -1; 
        }
        logfile.Write("connect database(%s) ok.\n", starg.connstr1);
    }

    // 数据抽取主函数
    _dminingmysql();

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/dminingmysql logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 3600 /project/tools/bin/dminingmysql /log/idc/dminingmysql_ZHOBTCODE.log \"<connstr>127.0.0.1,root,19981110,qxidc,3306</connstr><charset>utf8</charset><selectsql>select obtid,cityname,provname,lat,lon,height from T_ZHOBTCODE</selectsql><fieldstr>obtid,cityname,provname,lat,lon,height</fieldstr><fieldlen>10,30,30,10,10,10</fieldlen><bfilename>ZHOBTCODE</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><timeout>30</timeout><pname>dminingmysql_ZHOBTCODE</pname>\"\n\n");
    printf("       /project/tools/bin/procctl   30 /project/tools/bin/dminingmysql /log/idc/dminingmysql_ZHOBTMIND.log \"<connstr>127.0.0.1,root,19981110,qxidc,3306</connstr><charset>utf8</charset><selectsql>select obtid,date_format(ddatetime,'%%%%Y-%%%%m-%%%%d %%%%H:%%%%i:%%%%s'),t,p,u,wd,wf,r,vis,keyid from T_ZHOBTMIND where keyid>:1 and ddatetime>timestampadd(minute,-120,now())</selectsql><fieldstr>obtid,ddatetime,t,p,u,wd,wf,r,vis,keyid</fieldstr><fieldlen>10,19,8,8,8,8,8,8,8,15</fieldlen><bfilename>ZHOBTMIND</bfilename><efilename>HYCZ</efilename><outpath>/idcdata/dmindata</outpath><starttime></starttime><incfield>keyid</incfield><incfilename>/idcdata/dmining/dminingmysql_ZHOBTMIND_HYCZ.list</incfilename><timeout>30</timeout><pname>dminingmysql_ZHOBTMIND_HYCZ</pname><maxcount>1000</maxcount><connstr1>127.0.0.1,root,19981110,qxidc,3306</connstr1>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于从MySQL数据库源表抽取数据，生成xml文件。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("selectsql   从数据源数据库抽取数据的SQL语句，注意：时间函数的百分号%需要四个，显示出来才有两个，被prepare之后将剩一个。\n");
    printf("fieldstr    抽取数据的SQL语句输出结果集字段名，中间用逗号分隔，将作为xml文件的字段名。\n");
    printf("fieldlen    抽取数据的SQL语句输出结果集字段的长度，中间用逗号分隔。fieldstr与fieldlen的字段必须一一对应。\n");
    printf("bfilename   输出xml文件的前缀。\n");
    printf("efilename   输出xml文件的后缀。\n");
    printf("outpath     输出xml文件存放的目录。\n");
    printf("maxcount    输出xml文件的最大记录数，缺省是0，表示无限制，如果本参数取值为0，注意适当加大timeout的取值，防止程序超时。\n");
    printf("starttime   程序运行的时间区间，例如02,13表示：如果程序启动时，踏中02时和13时则运行，其它时间不运行。"\
                        "如果starttime为空，那么starttime参数将失效，只要本程序启动就会执行数据抽取，为了减少数据源"\
                        "的压力，从数据库抽取数据的时候，一般在对方数据库最闲的时候时进行。\n");
    printf("incfield    递增字段名，它必须是fieldstr中的字段名，并且只能是整型，一般为自增字段。"\
                        "如果incfield为空，表示不采用增量抽取方案。");
    printf("incfilename 已抽取数据的递增字段最大值存放的文件，如果该文件丢失，将重新抽取全部的数据。\n");
    printf("connstr1    已抽取数据的递增字段最大值存放的数据库的连接参数。connstr1和incfilename二选一，connstr1优先。");
    printf("timeout     本程序的超时时间，单位：秒。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {
    CCmdStr  CmdStr;    // 字符串拆分类    
    
    memset(&starg, 0, sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
    if (strlen(starg.connstr) == 0) { 
        logfile.Write("connstr is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
    if (strlen(starg.charset) == 0) { 
        logfile.Write("charset is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "selectsql", starg.selectsql, 1000);
    if (strlen(starg.selectsql) == 0) { 
        logfile.Write("selectsql is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "fieldstr", starg.fieldstr, 500);
    if (strlen(starg.fieldstr) == 0) { 
        logfile.Write("fieldstr is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "fieldlen", starg.fieldlen, 500);
    if (strlen(starg.fieldlen) == 0) { 
        logfile.Write("fieldlen is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "bfilename", starg.bfilename, 30);
    if (strlen(starg.bfilename) == 0) { 
        logfile.Write("bfilename is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "efilename", starg.efilename, 30);
    if (strlen(starg.efilename) == 0) { 
        logfile.Write("efilename is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "outpath", starg.outpath, 300);
    if (strlen(starg.outpath) == 0) { 
        logfile.Write("outpath is null.\n"); 
        return false; 
    }

    // 可选参数
    GetXMLBuffer(strxmlbuffer, "starttime", starg.starttime, 50);

    // 可选参数
    GetXMLBuffer(strxmlbuffer, "incfield", starg.incfield, 30);

    // 可选参数
    GetXMLBuffer(strxmlbuffer, "incfilename", starg.incfilename, 300);  

    // 可选参数
    GetXMLBuffer(strxmlbuffer,"maxcount",&starg.maxcount); 

    // 可选参数
    GetXMLBuffer(strxmlbuffer,"connstr1",starg.connstr1,100);

    // 进程心跳的超时时间
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);   
    if (starg.timeout == 0) { 
        logfile.Write("timeout is null.\n");  
        return false; 
    }
    
    // 进程名
    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);     
    if (strlen(starg.pname) == 0) { 
        logfile.Write("pname is null.\n");  
        return false; 
    }

    // 1. 把starg.fieldlen解析到ifieldlen数组中
    CmdStr.SplitToCmd(starg.fieldlen, ",", true);
    
    // 判断字段数是否超出MAXFIELDCOUNT的限制
    if (CmdStr.CmdCount() > MAXFIELDCOUNT) {
        logfile.Write("fieldlen的字段数太多, 超出了最大限制%d.\n", MAXFIELDCOUNT);
        return false;
    }
    for (int i = 0; i < CmdStr.CmdCount(); i++) {
        CmdStr.GetValue(i, &ifieldlen[i]);
        if (ifieldlen[i] > MAXFIELDLEN) 
            MAXFIELDLEN = ifieldlen[i];   // 字段的长度最大不能超过MAXFIELDLEN
    }
    
    ifieldcount = CmdStr.CmdCount();

    // 2. 把starg.fieldstr解析到strfieldname中
    CmdStr.SplitToCmd(starg.fieldstr, ",", true);

    // 判断字段数是否超出MAXFIELDCOUNT的限制
    if (CmdStr.CmdCount() > MAXFIELDCOUNT) {
        logfile.Write("fieldlen的字段数太多, 超出了最大限制%d.\n", MAXFIELDCOUNT);
        return false;
    }

    for (int i = 0; i < CmdStr.CmdCount(); i++) 
        CmdStr.GetValue(i, strfieldname[i], 30);
    
    // 判断strfieldname和ifieldlen两个数组中的字段数是否一致
    if (ifieldcount != CmdStr.CmdCount()) {
        logfile.Write("fieldstr 和 fieldlen 的元素数量不一致.\n");
        return false;
    }

    // 3. 获取自增字段在结果集中的位置
    if (strlen(starg.incfield) != 0) {
        for (int i = 0; i < ifieldcount; i++) {
            if (strcmp(starg.incfield, strfieldname[i]) == 0) {
                incfieldpos = i;
                break;
            }       
        }   

        if (incfieldpos == -1) {
            logfile.Write("递增字段名%s不在列表%s中.\n", starg.incfield, starg.fieldstr);
            return false;
        } 

        if ( (strlen(starg.incfilename) == 0) && (strlen(starg.connstr1) == 0) ) {
            logfile.Write("incfilename和connstr1参数必须二选一.\n"); 
            return false;
        }
    }






    return true;
}


bool inStartTime() {

    // 获取当前系统时间
    char strnowhour[5]; memset(strnowhour, 0, sizeof(strnowhour));
    LocalTime(strnowhour, "hh24");

    // 判断starg.starttime是否为空
    if (strlen(starg.starttime) == 0) return true;

    // 程序运行的时间区间, 例如 02,13 表示: 如果程序启动时, 踏中02时和13时运行, 其他时间不运行
    if (strstr(starg.starttime, strnowhour) == 0) return false;

    return true;
}


bool _dminingmysql() {
    
    CFile          File;              // 文件操作类
    sqlstatement   stmt(&conn);

    char strfieldvalue[ifieldcount][MAXFIELDLEN + 1];     // 抽取数据的SQL执行后, 存放结果集字段值的数组

    // 从starg.incfilename文件中获取已抽取数据的最大id
    readIncField();

    // 准备SQL语句
    stmt.prepare(starg.selectsql);

    // 更新进程的心跳
    PActive.UptATime();

    // 为SQL语句绑定输出参数
    for (int i = 0; i < ifieldcount; i++) {
        stmt.bindout(i + 1, strfieldvalue[i], ifieldlen[i]);
    }
    
    // 如果是增量抽取, 绑定输入参数(已抽取数据的最大id)
    if (strlen(starg.incfield) != 0) 
        stmt.bindin(1, &imaxincvalue);

    // 执行SQL语句 
    if (stmt.execute() != 0) {
        logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
        return false;
    }

    while (true) {
        memset(strfieldvalue, 0, sizeof(strfieldvalue));

        // 获取结果集
        if (stmt.next() != 0) break;

        if (!File.IsOpened()) {
            // 生成文件名
            crtXmlFileName();

            // 打开文件
            if (!File.OpenForRename(strxmlfilename, "w+")) {
                logfile.Write("File.OpenForRename(%s) failed.\n", strxmlfilename);
                return false; 
            }

            // 写入文件头部信息
            File.Fprintf("<data>\n");
        }

        // 把结果集写入文件
        for (int i = 0; i < ifieldcount; i++) {
            File.Fprintf("<%s>%s</%s>", strfieldname[i], strfieldvalue[i], strfieldname[i]);
        } 
        File.Fprintf("<endl/>\n");

        // 如果文件的记录数达到了starg.maxcount行就切换一个xml文件
        if (starg.maxcount > 0 && stmt.m_cda.rpc%starg.maxcount == 0) {
            // 写入文件尾部信息
            File.Fprintf("</data>\n");

            // 关闭文件
            if (!File.CloseAndRename()) {
                logfile.Write("File.CloseAndRename() failed.\n");
                return false;
            }

            logfile.Write("生成文件%s(%d).\n",strxmlfilename,starg.maxcount);

            // 更新进程的心跳
            PActive.UptATime();
        }

        // 更新自增字段的最大值
        if (strlen(starg.incfield) != 0 && atol(strfieldvalue[incfieldpos]) > imaxincvalue)
            imaxincvalue = atol(strfieldvalue[incfieldpos]);        
    }

    if (File.IsOpened()) {
        // 写入文件尾部信息
        File.Fprintf("</data>\n");

        // 关闭文件
        if (!File.CloseAndRename()) {
            logfile.Write("File.CloseAndRename() failed.\n");
            return false;
        }

        if (starg.maxcount == 0)
            logfile.Write("生成文件%s(%d).\n", strxmlfilename, stmt.m_cda.rpc);
        else
            logfile.Write("生成文件%s(%d).\n", strxmlfilename, stmt.m_cda.rpc%starg.maxcount);
    }

    // 把最大的自增字段的值写入starg.incfilename文件中
    if (stmt.m_cda.rpc > 0)
        writeIncField();

    return true;
}


bool readIncField() {

    CFile    File;    // 文件操作类

    int imaxincvalue = 0;        // 自增字段的最大值

    // 如果starg.incfield为空, 表示不是增量抽取
    if (strlen(starg.incfield) == 0) return true;

    if (strlen(starg.connstr1) != 0) {
        // 从数据库表中加载自增字段的最大值
        // create table T_MAXINCVALUE(pname varchar(50), maxincvalue numeric(15), primary key(pname));
        sqlstatement stmt(&conn1);
        stmt.prepare("select maxincvalue from T_MAXINCVALUE where pname = :1");
        stmt.bindin(1, starg.pname, 50);
        stmt.bindout(1, &imaxincvalue);
        stmt.execute();

        // 获取结果集
        stmt.next();        
    } else {
        // 从文件中加载自增字段的最大值
        if (!File.Open(starg.incfilename, "r")) return true;

        // 从文件中读取已抽取数据的最大id
        char strtemp[31]; memset(strtemp, 0, sizeof(strtemp));

        // 读取抽取数据的最大id
        File.Fgets(strtemp, 30, true);

        imaxincvalue = atol(strtemp);

        logfile.Write("上次已抽取数据的位置(%s = %ld).\n", starg.incfield, imaxincvalue);
    }

    return true;
}


void crtXmlFileName() {

    // 获取当前时间
    char now[15]; memset(now, 0, sizeof(now));
    LocalTime(now, "yyyymmddhh24miss");

    // 生成序号
    static int iseq = 1; 

    // xml全路径文件名: starg.outpath + starg.bfilename + 当前时间 + starg.efilename + 序号 + .xml
    SPRINTF(strxmlfilename, sizeof(strxmlfilename), "%s/%s_%s_%s_%d.xml", starg.outpath, starg.bfilename, now, starg.efilename, iseq);

    iseq++;
}


bool writeIncField() {
    CFile    File;    // 文件操作类

    // 如果starg.incfield为空, 表示不是增量抽取
    if (strlen(starg.incfield) == 0) return true;

    if (strlen(starg.connstr1) != 0) {
        // 把自增字段的最大值写入数据库的表
        sqlstatement stmt(&conn1);
        stmt.prepare("updata T_MAXINCVALUE set maxincvalue = :1 where pname = :2");

        // 判断表是否存在, 如果不存在就创建它
        if (stmt.m_cda.rc == 1064) {
            cout << 1 <<endl;
            conn1.execute("create table T_MAXINCVALUE(pname varchar(50), maxincvalue numeric(15), primary key(pname))");
            conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)", starg.pname, imaxincvalue);
            conn1.commit();
            return true;
        }
        stmt.bindin(1, &imaxincvalue);
        stmt.bindin(2, starg.pname, 50);
        if (stmt.execute() != 0) {
            logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message); 
            return false;
        }
        
        if (stmt.m_cda.rpc == 0) {
            conn1.execute("insert into T_MAXINCVALUE values('%s',%ld)",starg.pname,imaxincvalue);
        }
        // 提交事务
        conn1.commit();
    } else {
        // 把自增字段的最大值写入文件
        if (!File.Open(starg.incfilename, "w")) {
            logfile.Write("File.Open(%s) failed.\n", starg.incfilename);
            return false;
        }

        // 写入已抽取数据的最大id
        File.Fprintf("%d\n", imaxincvalue);

        File.Close();
    }

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}