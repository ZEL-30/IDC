/*
 *  程序名: xmltodb.cpp, 本程序是数据中心的公共功能模块, 用于把XML文件入库到MySQL的表中
 *  作者: ZEL
 */
#include "_tools.h"


#define  MAXCOLCOUNT 500       // 每个表字段的最大数


CLogFile     logfile;          // 日志文件操作类
CPActive     PActive;          // 进程操作类   
connection   conn;             // MySQL数据库连接类
CTABCOLS     TABCOLS;          // 获取表全部列和主键列信息的类
sqlstatement stmtins, stmtupt; // 插入和更新表的sqlstatement对象


// 程序参数的结构体
struct st_arg {
  char connstr[101];            // 数据库的连接参数
  char charset[51];             // 数据库的字符集
  char inifilename[301];        // 数据入库的参数配置文件
  char xmlpath[301];            // 待入库xml文件存放的目录
  char xmlpathbak[301];         // xml文件入库后的备份目录
  char xmlpatherr[301];         // 入库失败的xml文件存放的目录
  int  timetvl;                 // 本程序运行的时间间隔, 本程序常驻内存
  int  timeout;                 // 本程序运行时的超时时间
  char pname[51];               // 本程序运行时的程序名
} starg;


struct st_xmltotable {
    char filename[301];          // XML文件的匹配规则, 用逗号分隔
    char tname[31];              // 待入库的表名
    int  uptbz;                  // 更新标志: 1-更新; 2-不更新
    char execsql[301];           // 处理XML文件后, 执行的SQL语句
} stxmltotable;


vector<struct st_xmltotable> vxmltotable;     // 数据入库的参数的容器

char strInsertSql[10241];                     // 插入表的SQL语句
char strUpdateSql[10241];                     // 更新表的SQL语句
char *strcolvalue[MAXCOLCOUNT];               // 存放从XML每一行中解析出来的值
int  totalcount;                              // XML文件的总记录数
int  inscount;                                // XML文件的插入记录数
int  uptcount;                                // XML文件的更新记录数 



// 程序帮助文档
void _help();

// 把XML解析到starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 数据入库主函数
bool _xmltodb();

// 把数据入库的参数配置文件starg.inifilename加载到vxmltotable容器中
bool loadXmlToTable();

// 调用处理XML文件的子函数, 返回值: 0-成功, 其他的都是失败, 失败的情况有很多种, 暂时不确定
int _xmltodb(const char *fullfilename, const char *filename);

// 把XML文件移动到参数指定的目录中
bool xmltobakerr(const char *fullfilename, const char *srcpath, const char *dstpath);

// 从vxmltotable容器中查找xmlfilename的入库参数, 存放在stxmltotable结构体中
bool findXmlToTable(const char *xmlfilename);

// 拼接生成插入和更新表数据的SQL
void crtSQL();

// prepare插入和更新的SQL语句, 绑定输入变量
void prepareSQL();

// 在处理XML文件之前, 如果stxmltotable.execsql不为空, 就执行它
bool execsql();

// 解析XML, 存放在已绑定的输入变量中
void splitbuffer(const char *strBuffer);

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

    // 把XML解析到参数starg结构体中
    if (!_xmltoarg(argv[2])) return -1;    

    // 把进程的心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    // 数据入库主函数
    _xmltodb();

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/xmltodb logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/xmltodb /log/idc/xmltodb_vip1.log \"<connstr>127.0.0.1,root,19981110,qxidc,3306</connstr><charset>utf8</charset><inifilename>/project/tools/ini/xmltodb.xml</inifilename><xmlpath>/idcdata/xmltodb/vip1</xmlpath><xmlpathbak>/idcdata/xmltodb/vip1bak</xmlpathbak><xmlpatherr>/idcdata/xmltodb/vip1err</xmlpatherr><timetvl>5</timetvl><timeout>50</timeout><pname>xmltodb_vip1</pname>\"\n\n");

    printf("本程序是数据中心的公共功能模块，用于把xml文件入库到MySQL的表中。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：ip,username,password,dbname,port。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("inifilename 数据入库的参数配置文件。\n");
    printf("xmlpath     待入库xml文件存放的目录。\n");
    printf("xmlpathbak  xml文件入库后的备份目录。\n");
    printf("xmlpatherr  入库失败的xml文件存放的目录。\n");
    printf("timetvl     本程序的时间间隔，单位：秒，视业务需求而定，2-30之间。\n");
    printf("timeout     本程序的超时时间，单位：秒，视xml文件大小而定，建议设置30以上。\n");
    printf("pname       进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {

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

    GetXMLBuffer(strxmlbuffer, "inifilename", starg.inifilename, 300);
    if (strlen(starg.inifilename) == 0) { 
        logfile.Write("inifilename is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "xmlpath", starg.xmlpath, 300);
    if (strlen(starg.xmlpath) == 0) { 
        logfile.Write("xmlpath is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "xmlpathbak", starg.xmlpathbak, 300);
    if (strlen(starg.xmlpathbak) == 0) { 
        logfile.Write("xmlpathbak is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "xmlpatherr", starg.xmlpatherr, 300);
    if (strlen(starg.xmlpatherr) == 0) { 
        logfile.Write("xmlpatherr is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
    if (starg.timetvl < 2) starg.timetvl = 2;   
    if (starg.timetvl > 30) starg.timetvl = 30;

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


bool _xmltodb() {

    CDir    Dir;         // 目录操作类

    int  counter = 50;   // 加载入局参数的计数器, 初始化为50是为了在第一次进入循环的时候就加载参数

    // 本程序是常驻内存的, 所以用一个while循环
    while (true) {

        if (counter++ > 30) {
            // 重新计数
            counter = 0;

            // 把数据入库的参数配置文件starg.inifilename加载到容器中
            if (!loadXmlToTable()) {
                logfile.Write("loadXmlToTable() failed.\n");
                return false;
            }
        }


        // 打开starg.xmlpath目录, 注意OpenDir的第五个参数, 需要把文件进行排序, 因为先生成的文件先入库
        if (!Dir.OpenDir(starg.xmlpath, "*.xml", 10000, false, true)) {
            logfile.Write("Dir.OpenDir(%s) failed.\n", starg.xmlpath);
            return false;
        }

        while (true) {

            // 读取目录, 得到一个XML文件
            if (!Dir.ReadDir()) break;

            // 如果数据库未连接, 连接数据库
            if (conn.m_state == 0) {
                if (conn.connecttodb(starg.connstr, starg.charset) != 0) {
                    logfile.Write("connect database(%s) failed.\n", starg.connstr);
                    return false;
                }
                logfile.Write("connect database(%s) ok.\n", starg.connstr);
            }
            logfile.Write("处理文件%s ... ", Dir.m_FullFileName);

            // 调用处理XML文件的子函数
            int iret = _xmltodb(Dir.m_FullFileName, Dir.m_FileName);

            // 更新心跳信息
            PActive.UptATime();

            // 如果处理文件成功, 写日志, 备份文件
            if (iret == 0) {
                logfile.WriteEx(" ok(%s, total = %d, insert = %d, update = %d).\n", stxmltotable.tname, totalcount, inscount, uptcount);

                // 把XML文件移动到starg.xmlpathbak参数指定的目录中
                if (!xmltobakerr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpathbak)) {
                    logfile.Write("xmltobakerr(%s) failed.\n", starg.xmlpathbak);
                    return false;
                }
            }

            // 如果处理文件失败
            // 第一种情况: 找不到入库参数或者带入库的表不存在, 写日志, 把文件转存到starg.xmlpatherr目录中
            if (iret == 1 || iret == 2 || iret == 5) {
                if (iret == 1) logfile.WriteEx(" failed. 没有配置入库参数.\n");
                if (iret == 2) logfile.WriteEx(" failed. 带入库的表(%s)不存在.\n", stxmltotable.tname);
                if (iret == 5) logfile.WriteEx(" failed. 带入库的表(%s)字段数太多.\n", stxmltotable.tname);

                // 把XML文件移动到starg.xmlpatherr参数指定的目录中
                if (!xmltobakerr(Dir.m_FullFileName, starg.xmlpath, starg.xmlpatherr)) {
                    logfile.WriteEx("xmltobakerr(%s) failed.\n", starg.xmlpatherr);
                    return false;
                }
            }

            // 打开XML文件错误, 这种错误一般不会发生, 发生了程序退出
            if (iret == 3) {
                logfile.WriteEx("failed. 打开XML文件错误.\n");
                return false;
            }

            // 数据库错误, 函数返回, 程序退出
            if (iret == 4) {
                logfile.WriteEx("failed. 数据库错误.\n");
                return false;
            }

            // 在处理XML文件之前, 如果执行strxmltotable.execsql失败, 函数返回, 程序退出
            if (iret == 6) {
                logfile.WriteEx("failed. 执行SQL错误.\n");
                return false;
            }


        }
        
        // 如果刚才这次扫描到了有文件, 表示不空闲, 可能不断的有文件生成, 就不sleep了
        if (Dir.m_vFileName.size() == 0)
            sleep(starg.timetvl);

        // 更新心跳信息
        PActive.UptATime();
    }

    return true;
}


bool loadXmlToTable() {

    CFile   File;      // 文件操作类

    vxmltotable.clear();

    // 打开文件
    if (!File.Open(starg.inifilename, "r")) {
        logfile.Write("File.Open(%s) faield.\n", starg.inifilename);
        return false;
    }

    char strBuffer[1024];    // 存放读取出来的buffer

    while (true) {
        memset(&stxmltotable, 0, sizeof(struct st_xmltotable));
        memset(strBuffer, 0, sizeof(strBuffer));

        // 读取文件
        if (!File.FFGETS(strBuffer, 1000, "<endl/>")) break;

        // 解析XML,得到需要的字段
        GetXMLBuffer(strBuffer, "filename", stxmltotable.filename, 300);
        GetXMLBuffer(strBuffer, "tname", stxmltotable.tname, 30);
        GetXMLBuffer(strBuffer, "uptbz", &stxmltotable.uptbz);
        GetXMLBuffer(strBuffer, "execsql", stxmltotable.execsql, 300);

        vxmltotable.push_back(stxmltotable);

    }

    logfile.Write("loadxmltotable(%s) ok.\n", starg.inifilename);

    return true;
}


int _xmltodb(const char *fullfilename, const char *filename){    

    CFile   File;

    totalcount = inscount = uptcount = 0;

    // 释放上次处理XML文件时为字段分配的内存
    for (int i = 0; i < TABCOLS.m_allcount; i++) 
        if (strcolvalue[i] != NULL) {
            delete strcolvalue[i]; 
            strcolvalue[i] = NULL;
        }

    // 从vxmltotable容器中查找filename的入库参数, 存放在stxmltotable结构体中
    if (!findXmlToTable(filename)) return 1;

    // 获取表全部的字段和主键信息, 如果获取失败, 应该是数据库连接已失效
    // 在本程序运行的过程中, 如果数据库出现异常, 一定会在这里发现
    if (!TABCOLS.allcols(&conn, stxmltotable.tname)) return 4;
    if (!TABCOLS.pkcols(&conn, stxmltotable.tname))  return 4;

    // 如果TABCOLS.m_allcount为0, 说明表不存在, 返回2
    if (TABCOLS.m_allcount == 0) return 2;

    // 判断表的字段不能超过MAXCOLCOUNT
    if (TABCOLS.m_allcount > MAXCOLCOUNT) return 5;

    // 为每个字段分配内存
    for (int i = 0; i < TABCOLS.m_allcount; i++) 
        strcolvalue[i] = new char[TABCOLS.m_vallcols[i].collen + 1];

    // 拼接生成插入和更新表数据的SQL
    crtSQL();

    // prepare插入和更新的SQL语句, 绑定输入变量
    prepareSQL();

    // 在处理XML文件之前, 如果stxmltotable.execsql不为空, 就执行他
    if (!execsql())  return 6;

    // 打开XML文件
    if (!File.Open(fullfilename, "r")) {
        conn.rollback();
        return 3;
    }

    char strBuffer[10241];

    while (true) {

        // 从XML文件中读取一行
        if (!File.FFGETS(strBuffer, 10240, "<endl/>")) break;
        
        totalcount++;

        // 解析XML, 存放在已绑定的输入变量中
        splitbuffer(strBuffer);

        // 执行插入和更新的SQL 
        if (stmtins.execute() != 0) {
            // 违反唯一性约束, 表示记录已存在
            if (stmtins.m_cda.rc == 1062) {
                // 判断入参数的更新标志
                if (stxmltotable.uptbz == 1) {
                    if (stmtupt.execute() != 0) {
                        // 如果update失败, 记录出错的行和错误内容, 函数不返回, 继续处理数据, 也就是说, 不理这一行
                        logfile.Write("%s\n", strBuffer);
                        logfile.Write("stmtupt.execute() failed.\n%s\n%s\n", stmtupt.m_sql, stmtupt.m_cda.message);

                        // 数据库连接已失效, 无法继续, 只能返回
                        // 1053-在操作过程中服务器关闭, 2013-查询过程中丢失了与MySQL服务器的连接
                        if (stmtupt.m_cda.rc == 1053 || stmtupt.m_cda.rc == 2013) return 4;
                    } else uptcount++;
                }
            } else {
                // 如果insert失败, 记录出错的行和错误内容, 函数不返回, 继续处理数据, 也就是说, 不理这一行
                logfile.Write("%s\n", strBuffer);
                logfile.Write("stmtins.execute() failed.\n%s\n%s\n", stmtins.m_sql, stmtins.m_cda.message);

                // 数据库连接已失效, 无法继续, 只能返回
                // 1053-在操作过程中服务器关闭, 2013-查询过程中丢失了与MySQL服务器的连接
                if (stmtins.m_cda.rc == 1053 || stmtins.m_cda.rc == 2013) return 4;
            }

        } else inscount++;

    }   
    
    conn.commit();

    return 0;
} 


bool xmltobakerr(const char *fullfilename, const char *srcpath, const char *dstpath) {

    // 目标文件名
    char dstfilename[301]; memset(dstfilename, 0, sizeof(dstfilename));
    STRCPY(dstfilename, sizeof(dstfilename), fullfilename);
    UpdateStr(dstfilename, srcpath, dstpath, false);  // 第四个参数要小心, 一定要填false

    // 参数指定的文件
    if (!RENAME(fullfilename, dstfilename, 2)) {
        logfile.Write("RENAME(%s, %s) failed.\n", fullfilename, dstfilename);
        return false;
    }

    return true;
}


bool findXmlToTable(const char *xmlfilename) {

    for (int i = 0; i < vxmltotable.size(); i++) {
        if (MatchStr(xmlfilename, vxmltotable[i].filename)) {
            memcpy(&stxmltotable, &vxmltotable[i], sizeof(struct st_xmltotable));
            return true;
        }
    }

    return false;
}


void crtSQL() { 
    memset(strInsertSql, 0, sizeof(strInsertSql));
    memset(strUpdateSql, 0, sizeof(strUpdateSql));

    // 插入表的SQL语句, insert into 表名(%s) values(%s)
    char strinsertp1[3001];  // insert语句的字段列表
    char strinsertp2[3001];  // insert语句values后的内容

    memset(strinsertp1, 0, sizeof(strinsertp1));
    memset(strinsertp2, 0, sizeof(strinsertp2));
    
    char strtemp[501];
    int  colseq = 1;     // value部分字段的序号

    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {
        // upttime和keyid这两个字段不处理
        if (strcmp(TABCOLS.m_vallcols[i].colname, "upttime") == 0 || 
            strcmp(TABCOLS.m_vallcols[i].colname, "keyid") == 0) continue;

        // 拼接strinsertp1
        strcat(strinsertp1, TABCOLS.m_vallcols[i].colname);
        strcat(strinsertp1, ","); 

        // 拼接strinsetp2, 注意需要区分data字段和非data字段
        if (strcmp(TABCOLS.m_vallcols[i].datatype, "date") != 0) 
            SPRINTF(strtemp, sizeof(strtemp), ":%d", colseq);
        else
            SPRINTF(strtemp, sizeof(strtemp), "str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", colseq);

        strcat(strinsertp2, strtemp);
        strcat(strinsertp2, ",");

        colseq++;
    }

    // 删除strinsertp1中多余的逗号
    if (strlen(strinsertp1) > 0) strinsertp1[strlen(strinsertp1) - 1] = 0;

    // 删除strinsertp2中多余的逗号
    if (strlen(strinsertp2) > 0) strinsertp2[strlen(strinsertp2) - 1] = 0;

    // 拼接出完整的插入语句
    SNPRINTF(strInsertSql, 10240, sizeof(strInsertSql), "insert into %s(%s) values(%s)", stxmltotable.tname, strinsertp1, strinsertp2);

    // 如果入库参数中指定了表数据不需要更新, 就不生成update语句了, 函数返回
    if (stxmltotable.uptbz != 1) return;

    // 生成修改表的SQL语句
    // update T_ZHOBTMIND1 set t=:1,p=:2,u=:3,wd=:4,wf=:5,r=:6,vis=:7,upttime=now(),mint=:8,minttime=str_to_date(:9,'%Y%m%d%H%i%s') where obtid=:10 and ddatetime=str_to_date(:11,'%Y%m%d%H%i%s')

    // 更新TABCOLS.m_vallcols中的pkseq字段, 在拼接update语句的时候用到它
    for (int i = 0; i < TABCOLS.m_vpkcols.size(); i++)
        for (int j = 0; j < TABCOLS.m_vallcols.size(); j++)
            if (strcmp(TABCOLS.m_vallcols[j].colname, TABCOLS.m_vpkcols[i].colname) == 0) {
                // 更新m_vpkcols容器中的pkseq
                TABCOLS.m_vallcols[j].pkseq = TABCOLS.m_vpkcols[i].pkseq;
                break;
            }  

    // 先拼接update语句开始的部分
    sprintf(strUpdateSql, "update %s set ", stxmltotable.tname);

    int uptseq = 1;

    // 拼接update语句set后面的部分
    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {

        // keyid字段不需要处理
        if (strcmp(TABCOLS.m_vallcols[i].colname, "keyid") == 0) continue;

        // 如果是主键字段, 也不需要拼接在set的后面
        if (TABCOLS.m_vallcols[i].pkseq != 0) continue;

        // upttime字段直接等于now(), 这么做是为了考虑数据库的兼容性
        if (strcmp(TABCOLS.m_vallcols[i].colname, "upttime") == 0) {
            strcat(strUpdateSql,"upttime=now(),");
            continue;
        }

        // 注意需要区分data字段和非data字段
        if (strcmp(TABCOLS.m_vallcols[i].datatype, "date") != 0)
            SPRINTF(strtemp, sizeof(strtemp), "%s = :%d", TABCOLS.m_vallcols[i].colname, uptseq);
        else
            SPRINTF(strtemp, sizeof(strtemp), "%s = str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", TABCOLS.m_vallcols[i].colname, uptseq);

        strcat(strUpdateSql, strtemp);
        strcat(strUpdateSql, ",");
        uptseq++;
    }

    // 删除多余的逗号
    strUpdateSql[strlen(strUpdateSql) - 1] = 0;

    // 然后再拼接update语句where后面的部分
    strcat(strUpdateSql, " where 1 = 1 ");    // 用1 = 1 是为了后面的拼接方便, 这是常用的处理方法

    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {
        // 如果不是主键字段, 跳过
        if (TABCOLS.m_vallcols[i].pkseq == 0) continue;

        // 注意需要区分data字段和非data字段
        if (strcmp(TABCOLS.m_vallcols[i].datatype, "date") != 0)
            SPRINTF(strtemp, sizeof(strtemp), " and %s = :%d", TABCOLS.m_vallcols[i].colname, uptseq);
        else
            SPRINTF(strtemp, sizeof(strtemp), " and %s = str_to_date(:%d, '%%%%Y%%%%m%%%%d%%%%H%%%%i%%%%s')", TABCOLS.m_vallcols[i].colname, uptseq);

        strcat(strUpdateSql, strtemp);
        uptseq++; 
    }


}

                        
void prepareSQL() {

    // 绑定插入SQL语句的输入变量
    stmtins.connect(&conn);
    stmtins.prepare(strInsertSql);
    
    int  colseq = 1;     // values部分字段的序号
    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {
        // upttime和keyid这两个字段不处理
        if (strcmp(TABCOLS.m_vallcols[i].colname, "upttime") == 0 || 
            strcmp(TABCOLS.m_vallcols[i].colname, "keyid") == 0) continue;
        
        // 注意, strcolvalue数组的使用不是连续的, 是和TABCOLS.m_vallcols的下标一一对应的
        stmtins.bindin(colseq, strcolvalue[i], TABCOLS.m_vallcols[i].collen);

        colseq++;
    }

    // 如果入库参数中指定类表数据不更新, 就不处理update语句了, 函数返回
    if (stxmltotable.uptbz != 1) return;

    // 绑定更新sql语句的输入变量
    stmtupt.connect(&conn);
    stmtupt.prepare(strUpdateSql);

    colseq = 1;

    // 绑定set部分的输入参数
    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {
        // upttime和keyid这两个字段不处理
        if (strcmp(TABCOLS.m_vallcols[i].colname, "upttime") == 0 || 
            strcmp(TABCOLS.m_vallcols[i].colname, "keyid") == 0) continue;

        // 如果是主键字段, 也不需要绑定
        if (TABCOLS.m_vallcols[i].pkseq != 0) continue;

        // 注意, strcolvalue数组的使用不是连续的, 是和TABCOLS.m_vallcols的下标一一对应的
        stmtupt.bindin(colseq, strcolvalue[i], TABCOLS.m_vallcols[i].collen);

        colseq++;
    }

    // 绑定where部分的输入参数
    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {
        // 如果是主键字段, 也不需要绑定
        if (TABCOLS.m_vallcols[i].pkseq == 0) continue;

        // 注意, strcolvalue数组的使用不是连续的, 是和TABCOLS.m_vallcols的下标一一对应的
        stmtupt.bindin(colseq, strcolvalue[i], TABCOLS.m_vallcols[i].collen);

        colseq++;
    }

}


bool execsql() {

    if (strlen(stxmltotable.execsql) == 0) return true;

    if (conn.execute(stxmltotable.execsql) != 0) {
        logfile.Write("conn.execute(%s) failed.\n", stxmltotable.execsql, conn.m_cda.message);
        return false;
    } 

    return true;
}


void splitbuffer(const char *strBuffer) {

    // 初始化strcolvalue数组
    for (int i = 0; i < TABCOLS.m_allcount; i++) 
        memset(strcolvalue[i], 0, TABCOLS.m_vallcols[i].collen + 1);

    char strtemp[31];

    for (int i = 0; i < TABCOLS.m_vallcols.size(); i++) {

        // 如果是日期时间字段, 提取数值就可以了
        // 也就是说, XML文件中的日期时间只要包含了yyyymmddhh24miss就行, 可以是任意分隔符
        if (strcmp(TABCOLS.m_vallcols[i].datatype, "date") == 0) {
            GetXMLBuffer(strBuffer,TABCOLS.m_vallcols[i].colname, strtemp, TABCOLS.m_vallcols[i].collen);
            PickNumber(strtemp, strcolvalue[i], false, false);
            continue;
        } 

        // 如果是数值字段, 只提取数字、+、-、符号和圆点
        if (strcmp(TABCOLS.m_vallcols[i].datatype, "number") == 0) {
            GetXMLBuffer(strBuffer,TABCOLS.m_vallcols[i].colname, strtemp, TABCOLS.m_vallcols[i].collen);
            PickNumber(strtemp, strcolvalue[i], true, true);
            continue;
        } 

        // 如果是字符字段, 直接提取
        GetXMLBuffer(strBuffer,TABCOLS.m_vallcols[i].colname, strcolvalue[i], TABCOLS.m_vallcols[i].collen);
    }
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}
