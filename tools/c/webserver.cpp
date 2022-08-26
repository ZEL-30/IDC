/*
 *  程序名: webserver.cpp, 本程序是数据服务总线的服务端程序
 *  作者: ZEL
 *  http://124.70.51.197:5080/api?username=ty&passwd=typwd&intername=getzhobtmind1&obtid=59097&begintime=20211024094318&endtime=20231024114020
 *  http://10.211.55.3:5080/api?username=ty&passwd=typwd&intername=getzhobtmind1&obtid=59097&begintime=20211024094318&endtime=20231024114020
 *  wget "http://124.70.51.197:5080/api?username=ty&passwd=typwd&intername=getzhobtmind1&obtid=59097&begintime=20211024094318&endtime=20231024114020" -O tmp.txt
 */
#include "_connpool.h"

#define MAXCONNS 10       // 数据库连接池的大小

CTcpServer TcpServer;     // 网络通信服务端类
CLogFile   logfile;       // 日志文件操作类
CConnPool  orcconnpool;   // 数据库连接池类 

// 主程序参数的结构体
struct st_arg {
    char connstr[101];     // 数据库的连接参数
    char charset[51];      // 数据库的字符集
    int  port;             // WEB服务监听的端口
} starg;

// 线程信息的结构体
struct st_pthinfo {
    pthread_t   pthid;     // 线程编号
    time_t      atime;     // 最后一次活动时间
};



pthread_spinlock_t spin;              // 声明用于给vthid容器加锁的自旋锁
vector<struct st_pthinfo> vthid;      // 存放全部线程信息的容器

pthread_mutex_t mutexs[MAXCONNS];     // 数据库连接池的锁, 注意有多少个数据库连接, 就需要多少把锁, 而不是共用一把锁
connection conns[MAXCONNS];           // 数据库连接池的数组

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;    // 初始化互斥锁
pthread_cond_t  cond  = PTHREAD_COND_INITIALIZER;     // 初始化条件变量
vector<int> sockqueue;                                // 客户端socket的队列


pthread_t checkthid   = 0;            // 监控线程ID
pthread_t checkpoolid = 0;            // 检查数据库连接线程ID


char strsendbuffer[1024];             // 发送报文的buffer
char strrecvbuffer[1024];             // 接受报文的buffer


// 程序的帮助文档
void _help();

// 把XML文件解析到参数starg结构中
bool xmltoarg(const char *strxmlbuffer);

// 线程主函数
void *thmain(void *arg);

// 读取客户端的报文, 不带报文长短, 但是带有超时机制的接收函数
int ReadT(const int sockfd, char *buffer, const int size, const int itimeout);

// 判断URL中用户名和密码, 如果不正确, 返回认证失败的响应报文
bool Login(connection *conn, const int sockfd, const char *strrecvbuffer);

// 判断用户是否有调用接口的权限, 如果没有, 返回没有权限的响应报文
bool checkPrem(connection *conn, const int sockfd, const char *strrecvbuffer);

// 解析URL中的参数
bool getURL(const char *getstr, const char *name, char *value, const int len);

// 执行接口的SQL语句, 把数据发送给客户端
bool execSQL(connection *conn, const int sockfd, const char *strrecvbuffer);

// 初始化数据库连接池
bool initConns();

// 从数据库连接池中获取一个空闲的连接
connection *getConns();

// 释放/归还数据库连接
bool freeConns(connection *conn);

// 释放数据库连接占用的资源, 包括断开数据库连接和销毁锁
bool destroyConns();

// 检查数据库连接池的线程函数
void *checkpool(void *arg);

// 监控线程主函数
void *checkthmain(void *arg);

// 线程清理函数
void thcleanup(void *arg);

// 进程的信号处理函数
void EXIT(int sig);





int main(int argc, char const *argv[]) {
    
    if (argc != 3) {
        _help();
        return -1;
    }

    // 关闭全部信号和输入输出
    // 设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
    // 但请不要用 "kill -9 + 进程号" 强行终止
    CloseIOAndSignal();
    signal(SIGINT, EXIT);
    signal(SIGTERM, EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1], "a+")) {
        printf("logfile.Open(%d) failed.\n", argv[1]);
        return -1;
    }

    // 解析XML文件, 获得程序的帮助文档
    if (!xmltoarg(argv[2])) {
        printf("xmltoarg(%s) failed.\n", argv[2]);
        return -1;
    }
    
    // 初始化自旋锁
    pthread_spin_init(&spin, 0);

    // 初始化服务端
    if (!TcpServer.InitServer(starg.port)) {
        logfile.Write("TcpServer.InitServer(%s) failed.\n", starg.port);
        return -1;
    }

    // 初始化数据库连接池
    if (!orcconnpool.init(starg.connstr, starg.charset, 10, 30)) {
        logfile.Write("orcconnpool.init() failed.\n");
        return -1;
    } else {
        // 创建一个线程, 专门用于检查连接池
        if (pthread_create(&checkpoolid, NULL, checkpool, NULL) != 0) {
            logfile.Write("pthread_create() failed.\n");
            return -1;
        }
    }

    // 启动10个工作线程, 线程数比CPU核数略多
    for (int i = 0; i < 10; i++) {
        struct st_pthinfo stpthinfo; memset(&stpthinfo, 0, sizeof(struct st_pthinfo));

        if (pthread_create(&stpthinfo.pthid, NULL, thmain, (void *)(long)i) != 0) {
            logfile.Write("pthread_create() failed.\n");
            return -1;
        }

        stpthinfo.atime = time(NULL);   // 把线程的活动时间置为当前时间
        
        // 把线程ID保存到vthid容器中
        vthid.push_back(stpthinfo);
    }

    // 创建监控线程
    if (pthread_create(&checkthid, NULL, checkthmain, NULL) != 0) {
        logfile.Write("pthread_create() failed.\n");
        return -1;
    }


    while (true) {

        // 等待客户端连接
        if (!TcpServer.Accept()) {
            logfile.Write("TcpServer.Accept() failed.\n");
            return -1;
        }
        logfile.Write("客户端(%s)已连接\n", TcpServer.GetIP());

        // 把客户端的socket放入队列, 并发送条件信号
        pthread_mutex_lock(&mutex);          // 给缓存队列加锁
        sockqueue.push_back(TcpServer.m_connfd); 
        pthread_mutex_unlock(&mutex);        // 给缓存队列解锁

        // 激活全部线程
        // pthread_cond_broadcast(&cond);
        
        // 激活一个线程
        pthread_cond_signal(&cond);

    }

    return 0;
}



void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/webserver logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 10 /project/tools/bin/webserver /log/idc/webserver.log \"<connstr>qxidc/19981110@snorcl11g_197</connstr><charset>Simplified Chinese_China.AL32UTF8</charset><port>5080</port>\"\n\n");

    printf("本程序是数据总线的服务端程序，为数据中心提供http协议的数据访问接口。\n");
    printf("logfilename 本程序运行的日志文件。\n");
    printf("xmlbuffer   本程序运行的参数，用xml表示，具体如下：\n\n");

    printf("connstr     数据库的连接参数，格式：username/password@tnsname。\n");
    printf("charset     数据库的字符集，这个参数要与数据源数据库保持一致，否则会出现中文乱码的情况。\n");
    printf("port        web服务监听的端口。\n\n");
}


bool xmltoarg(const char *strxmlbuffer) {

    memset(&starg, 0, sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer, "connstr", starg.connstr, 100);
    if (strlen(starg.connstr) == 0) {
        logfile.Write("connstr is null.\n");
        return false;
    }
    
    GetXMLBuffer(strxmlbuffer, "charset", starg.charset, 50);
    if (strlen(starg.connstr) == 0) {
        logfile.Write("charset is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer, "port", &starg.port);
    if (starg.port == 0) {
        logfile.Write("port is null.\n");
        return false;
    }

    return true;
}


void *thmain(void *arg) {

    // 注册线程清理函数
    pthread_cleanup_push(thcleanup, arg);

    // 设置线程的取消方式为立即取消
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    // 分离线程, 让系统回收资源
    pthread_detach(pthread_self());

    int pthnum = (int)(long)arg;    // 线程编号
    int sockfd = 0;                 // 客户端的socket 
    char strrecvbuffer[1024];       // 接收客户端的请求报文的buffer
    char strsendbuffer[1024];       // 发送客户端的回应报文的buffer


    while (true) {

        pthread_mutex_lock(&mutex);         // 给缓存队列加锁

        // 如果缓存队列为空, 等待, 用while防止条件变量被虚假唤醒
        while (sockqueue.size() == 0) {
            // 获取当前时间
            struct timeval now;
            gettimeofday(&now, NULL);   

            // 取当前时间之后的20秒
            now.tv_sec += 20;

            // 等待条件被触发
            pthread_cond_timedwait(&cond, &mutex, (struct timespec *)&now);

            // 如果超时返回, 更新当前线程的活动时间
            vthid[pthnum].atime = time(NULL);  
        }

        // 从缓存队列中获取一条记录, 然后删除该记录
        sockfd = sockqueue[0];
        sockqueue.erase(sockqueue.begin());

        pthread_mutex_unlock(&mutex);       // 给缓存队列解锁

        // 以下是业务处理的代码
        logfile.Write("pthid = %ld(%d), sockfd = %d\n", pthread_self(), pthnum, sockfd);

        // 不能用原生的recv函数, 因为recv函数没有超时机制,服务端不可能一直等
        // 注意: 超时时间不宜过长, 一般2-3秒
        // 读取客户端的报文, 如果超时或失败, 继续回到循环
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        if (ReadT(sockfd, strrecvbuffer, sizeof(strrecvbuffer), 3) <= 0) {
            close(sockfd);    // 关闭客户端socket
            continue;
        }

        // 如果不是GET请求报文不处理, 继续回到循环
        if (strncmp(strrecvbuffer, "GET", 3) != 0) {
            close(sockfd);    // 关闭客户端socket
            continue;
        } 

        // 连接数据库
        connection *conn = orcconnpool.get();

        // 如果数据库连接为空, 返回内部错误
        if (conn == NULL) {
            memset(strsendbuffer, 0, sizeof(strsendbuffer));
            strcpy(strsendbuffer,\
                                "HTTP/1.1 200 OK\r\n"\
                                "Server: webserver\r\n"\
                                "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                                "<retcode>-1</retcode><message>internal error.</message>");
            Writen(sockfd, strsendbuffer, sizeof(strsendbuffer)); 
            close(sockfd);    // 关闭客户端socket
            continue;
        }

        // 判断URL中用户名和密码, 如果不正确, 返回认证失败的响应报文, 继续回到循环
        if (!Login(conn, sockfd, strrecvbuffer)) {
            orcconnpool.free(conn);
            close(sockfd);    // 关闭客户端socket
            continue;
        }

        // 判断用户是否有调用接口的权限, 如果没有, 放回没有权限的响应报文, 继续回到循环
        if (!checkPrem(conn, sockfd, strrecvbuffer)) {
            orcconnpool.free(conn);
            close(sockfd);    // 关闭客户端socket
            continue;
        }

        // 先把响应报文头部发送给客户端
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        strcpy(strsendbuffer,\
                            "HTTP/1.1 200 OK\r\n"\
                            "Server: webserver\r\n"\
                            "Content-Type: text/html;charset=utf-8\r\n\r\n");
        Writen(sockfd, strsendbuffer, sizeof(strsendbuffer));

        // 再执行接口的SQL语句名, 把数据返回给客户端
        if (!execSQL(conn, sockfd, strrecvbuffer)) {
            orcconnpool.free(conn);
            close(sockfd);    // 关闭客户端socket
            continue;
        } 


        orcconnpool.free(conn);
        close(sockfd);    // 关闭客户端socket
    }

    // 线程清理函数出栈
    pthread_cleanup_pop(1);

    return NULL;    
}


int ReadT(const int sockfd, char *buffer, const int size, const int itimeout) {

    // I/O复用技术
    if (itimeout > 0) {
        struct pollfd fds;
        fds.fd = sockfd;
        fds.events = POLLIN;
        int iret;
        if ((iret = poll(&fds, 1, itimeout*1000)) <= 0) return iret;
    }

    return recv(sockfd, buffer, size, 0);
}


bool Login(connection *conn, const int sockfd, const char *strrecvbuffer) {

    char username[31], passwd[31];
    memset(username, 0, sizeof(username));
    memset(passwd, 0, sizeof(passwd));

    getURL(strrecvbuffer, "username", username, 30);
    getURL(strrecvbuffer, "passwd", passwd, 30);

    // 查询T_USERINFO表, 判断用户名和密码是否存在
    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select count(*) from T_USERINFO where username = :1 and passwd = :2 and rsts = 1");
    stmt.bindin(1, username, 30);
    stmt.bindin(2, passwd, 30);
    int icount = 0;
    stmt.bindout(1, &icount);
    stmt.execute();
    stmt.next();

    // 认证失败, 返回认证失败的响应报文
    if (icount == 0) {
        // 向客户端发送错误信息
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        
        strcpy(buffer, \
                        "HTTP/1.1 200 OK\r\n"\
                        "Serber: webserver\r\n"\
                        "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                        "<!DOCTYPE html>  \
                        <html>  \
                            <head>  \
                                <meta charset=\"utf-8\">  \
                            </head>  \
                            <body>  \
                                <h1>错误!</h1>  \
                                <p>用户名或密码错误!</p>  \
                            </body>  \
                        </html>");
        Writen(sockfd, buffer, strlen(buffer)); 
        return false;
    }

    return true;
}


bool getURL(const char *getstr, const char *name, char *value, const int len) {

    char *start = NULL, *end = NULL;
    value[0] = 0;

    start = strstr((char *)getstr, (char *)name);

    end = strstr(start, "&");
    if (end == NULL) end = strstr(start, " ");
    if (end == NULL) return false;

    int ilen = strlen(start) - strlen(end) - strlen(name) - 1;
    if (ilen > len) ilen = len;

    strncpy(value, start + strlen(name) + 1, ilen);
    value[ilen] = 0;

    return true;
}


bool checkPrem(connection *conn, const int sockfd, const char *strrecvbuffer) {

    char username[31], intername[31];
    memset(username, 0, sizeof(username));
    memset(intername, 0, sizeof(intername));

    // 获取用户名和接口名
    getURL(strrecvbuffer, "username", username, 30);
    getURL(strrecvbuffer, "intername", intername, 30);

    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select count(*) from T_USERANDINTER where username = :1 and intername = :2 and intername in (select intername from T_INTERCFG where rsts = 1)");
    stmt.bindin(1, username, 30);
    stmt.bindin(2, intername, 30);
    int icount = 0;
    stmt.bindout(1, &icount);
    stmt.execute();
    stmt.next();

    if (icount != 1) {
        // 向客户端发送错误信息
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        
        strcpy(buffer, \
                        "HTTP/1.1 200 OK\r\n"\
                        "Serber: webserver\r\n"\
                        "Content-Type: text/html;charset=utf-8\r\n\r\n"\
                        "<!DOCTYPE html>  \
                        <html>  \
                            <head>  \
                                <meta charset=\"utf-8\">  \
                            </head>  \
                            <body>  \
                                <h1>错误!</h1>  \
                                <p>没有访问接口的权限!</p>  \
                            </body>  \
                        </html>");
        Writen(sockfd, buffer, strlen(buffer)); 
        return false;
    }


    return true;
}


bool execSQL(connection *conn, const int sockfd, const char *strrecvbuffer) {

    // 从请求报文中解析接口名
    char intername[31]; memset(intername, 0, sizeof(intername));
    getURL(strrecvbuffer, "intername", intername, 30);

    // 从接口参数配置表T_INTERCFG中加载接口参数
    char selectsql[1001], colstr[301], bindin[301];
    memset(selectsql, 0, sizeof(selectsql));      // 接口SQL
    memset(colstr, 0, sizeof(colstr));            // 输出列名
    memset(bindin, 0, sizeof(bindin));            // 接口参数

    sqlstatement stmt;
    stmt.connect(conn);
    stmt.prepare("select selectsql, colstr, bindin from T_INTERCFG where intername = :1");
    stmt.bindin(1, intername, 30);
    stmt.bindout(1, selectsql, 1000);
    stmt.bindout(2, colstr, 300);
    stmt.bindout(3, bindin, 300);
    stmt.execute();
    stmt.next();

    // 准备查询数据的SQL语句
    stmt.prepare(selectsql);

    // 绑定查询数据的SQL语句的输入变量
    // 根据接口配置中的参数列表(bindin字段), 从URL中解析出参数的值, 绑定到查询数据的SQL语句中
    ////////////////////////////////////////////////////////////////////////////
    // 拆分输出参数bindin
    CCmdStr CmdStr;
    CmdStr.SplitToCmd(bindin, ",");

    // 声明用于存放输入参数的数组, 输入参数的值不会太长, 100足够
    char invalue[CmdStr.CmdCount()][101];
    memset(invalue, 0, sizeof(invalue));

    // 从HTTP的GRT请求中解析出输入参数, 绑定到sql中
    for (int i = 1; i <= CmdStr.CmdCount(); i++) {
        getURL(strrecvbuffer, CmdStr.m_vCmdStr[i - 1].c_str(), invalue[i - 1], 100);
        stmt.bindin(i, invalue[i - 1], 100);
    }
    ////////////////////////////////////////////////////////////////////////////

    // 绑定查询数据的SQL语句的输出变量
    // 根据接口配置中的列名(colstr字段), bindout结果集
    ////////////////////////////////////////////////////////////////////////////
    // 拆分colstr, 可以得到结果集的字段集
    CmdStr.SplitToCmd(colstr, ",");

    // 用于存放结果集的数组
    char colvalue[CmdStr.CmdCount()][2001];

    // 把结果集绑定到colvalue数组
    for (int i = 1; i <= CmdStr.CmdCount(); i++) {
        stmt.bindout(i, colvalue[i - 1], 2000);
    }
    ////////////////////////////////////////////////////////////////////////////

    // 执行SQL语句
    char strhead[102400]; memset(strhead, 0, sizeof(strhead));
    if (stmt.execute() != 0) {
        SNPRINTF(strhead, 100000, sizeof(strhead), "<retcode>%d</retcode><message>%s</message>\n", stmt.m_cda.rc, stmt.m_cda.message);
        Writen(sockfd, strhead, sizeof(strhead));

        logfile.Write("stmt.execute() failed.\n%s\n%s\n", stmt.m_cda, stmt.m_cda.message);
        return false;
    }
    strcpy(strhead, "<retcode>0</retcode><message>ok</message>\n");
    Writen(sockfd, strhead, sizeof(strhead));

    // 向客户端发送xml内容的头部标签<data>
    strcpy(strhead,"<!DOCTYPE html>    \
                <html>   \
                <meta charset=\"utf-8\">   \
                <title>数据中心</title>   \
                <head>   \ 
                    <style type=\"text/css\">   \
                        table.altrowstable {   \
                            font-family: verdana,arial,sans-serif;   \
                            font-size:11px;   \
                            color:#333333;   \
                            border-width: 1px;   \
                            border-color: #a9c6c9;   \
                            border-collapse: collapse;   \
                        }   \
                        table.altrowstable th {   \
                            border-width: 1px;   \
                            padding: 8px;   \
                            border-style: solid;   \
                            border-color: #a9c6c9;   \
                        }   \
                        table.altrowstable td {   \
                            border-width: 1px;   \
                            padding: 8px;   \
                            border-style: solid;   \
                            border-color: #a9c6c9;   \
                        }   \
                        .oddrowcolor{   \
                            background-color:#d4e3e5;   \
                        }   \
                        .evenrowcolor{   \
                            background-color:#c3dde0;   \
                        }   \
                    </style>   \
                    <script type=\"text/javascript\">   \
                        function altRows(id){   \
                            if(document.getElementsByTagName){    \
                                    \
                                var table = document.getElementById(id);    \
                                var rows = table.getElementsByTagName(\"tr\");   \
                                for(i = 0; i < rows.length; i++){\   
                                    if(i % 2 == 0){   \
                                        rows[i].className = \"evenrowcolor\";   \
                                    }else{   \
                                        rows[i].className = \"oddrowcolor\";   \
                                    }\ 
                                }   \
                            }   \
                        }   \
                            \
                        window.onload=function(){   \
                            altRows('alternatecolor');   \
                        }   \
                </script>   \
                </head>   \
                <body>   \
                <!-- Table goes in the document BODY -->   \
    <table class=\"altrowstable\" id=\"alternatecolor\">" );
    Writen(sockfd, strhead, strlen(strhead));

    char strtemp[102400]; 
    char strcontent[102400];
    while (true) {
        memset(strcontent, 0, sizeof(strcontent));
        memset(colvalue, 0, sizeof(colvalue));

        // 获取结果集, 每获取一条记录, 拼接xml报文, 发送给客户端
        if (stmt.next() != 0) break;

        strcpy(strcontent, "<tr>\n");
        for (int i = 0; i < CmdStr.CmdCount(); i++) {
            memset(strtemp, 0, sizeof(strtemp));
            SPRINTF(strtemp, sizeof(strtemp), "<td>%s</td>", colvalue[i]);
            strcat(strcontent, strtemp);
        }
        strcat(strtemp, "\n</tr>\n");
        strcat(strcontent, strtemp);

        Writen(sockfd, strcontent, strlen(strcontent));
    }

    // 向客户端发送XML内容的尾部标签</data>
    char strtail[1024]; memset(strtail, 0, sizeof(strtail));
    strcpy(strtail," </table> \
                    </body>\
                    </html>\n");
    Writen(sockfd, strtail, strlen(strtail));

    // 写入接口调用日志表T_USERLOG
    logfile.Write("intername = %s, count = %d\n", intername, stmt.m_cda.rpc);

    return true;
}


bool initConns() {

    for (int i = 0; i < MAXCONNS; i++) {
        // 连接数据库
        if (conns[i].connecttodb(starg.connstr, starg.charset) != 0) {
            logfile.Write("connect database(%s) failed.\n%s\n", starg.connstr, conns[i].m_cda.message);
            return false;
        }

        // 初始化数据库互斥锁
        pthread_mutex_init(&mutexs[i], NULL);
    }    

    return true;
}


connection *getConns() {

    // 采用轮询的方法, 尝试对互斥锁进行加锁
    // 如果加锁成功, 返回这个锁所对应的数据库连接的地址
    // 如果失败, 过一段时间再次轮询

    while (true) {

        for (int i = 0; i < MAXCONNS; i++) {
            // 尝试加锁, 加锁成功返回地址
            if (pthread_mutex_trylock(&mutexs[i]) == 0) {
                logfile.Write("get conns is %d.\n", i);
                return &conns[i];
            }
        }

        usleep(10000);   // 百分之一秒之后再重试
    }
}


bool freeConns(connection *conn) {

    for (int i = 0; i < MAXCONNS; i++) {
        if (&conns[i] == conn) {
            // 释放数据库连接的互斥锁
            pthread_mutex_unlock(&mutexs[i]);
            conn = NULL;
            return true;
        }
    }    

    return false;
}


bool destroyConns() {

    for (int i = 0; i < MAXCONNS; i++) {
        // 断开数据库连接
        conns[i].disconnect();

        // 销毁数据库连接的互斥锁
        pthread_mutex_destroy(&mutexs[i]);
    }



    return true;
}



void *checkpool(void *arg) {
    while (true) {
        orcconnpool.checkpool();

        sleep(30);
    }
}


void *checkthmain(void *arg) {

    while (true) {

        // 遍历工作线程ID的容器, 检查每个工作线程是否超时
        for (int i = 0; i < vthid.size(); i++) {
            // 工作线程超时时间为20秒, 这里用25秒判断超时, 足够
            if (time(NULL) - vthid[i].atime > 25) {
                logfile.Write("thread %d(%lu) timeout(%d).\n", i, vthid[i].pthid, time(NULL) - vthid[i].atime);

                // 取消已超时的工作线程
                pthread_cancel(vthid[i].pthid);

                // 重新创建工作线程
                if (pthread_create(&vthid[i].pthid, NULL, thmain, (void *)(long)i) != 0) {
                    logfile.Write("pthread_create() failed.\n");
                    EXIT(-1);
                }

                vthid[i].atime = time(NULL);    // 设置工作线程的活动时间
            }
        } 

        sleep(2);
    }


    return NULL;  
}


void thcleanup(void *arg) {

    pthread_mutex_unlock(&mutex);      // 解开互斥锁, 否则会导致线程无法正常退出

    // 把本线程ID从存放线程ID的容器中删除
    // 注意!!! 如果线程跑的太快, 主程序会来不及把线程ID放入容器
    // 所以, 这里可能会找不到线程ID的情况  
    pthread_spin_lock(&spin);           // 加锁
    for (int i = 0; i < vthid.size(); i++) {
        if (pthread_equal(vthid[i].pthid, pthread_self()) == 0) {
            vthid.erase(vthid.begin() + i);
            break;
        }
    }
    pthread_spin_unlock(&spin);         // 解锁  

    logfile.Write("线程(%d)%lu退出.\n", (int)(long)arg, pthread_self());
}


void EXIT(int sig) {

    logfile.Write("程序退出, sig = %d\n", sig);

    // 关闭监听的socket
    TcpServer.CloseListen();

    // 取消全部工作线程
    pthread_spin_lock(&spin);       // 加锁
    for (int i = 0; i < vthid.size(); i++) {
        // 注意!!! 如果线程跑的太快, 主程序可能还来不及把线程的ID放入容器
        // 线程清理函数可能没有来得及从容器中删除自己的ID
        // 所以, 以下代码可能会出现段错误
        pthread_cancel(vthid[i].pthid);
    }
    pthread_spin_unlock(&spin);     // 解锁


    // 取消监控线程
    pthread_cancel(checkthid);

    // 取消检查数据库连接池的线程
    pthread_cancel(checkpoolid);

    // 让子线程有足够的时间退出
    sleep(1);

    pthread_spin_destroy(&spin);      // 销毁自旋锁
    pthread_mutex_destroy(&mutex);    // 销毁互斥锁
    pthread_cond_destroy(&cond);      // 销毁条件变量
    
    exit(0);
}

