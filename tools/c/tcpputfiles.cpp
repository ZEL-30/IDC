/*
 *  程序名: tcpputfiles.cpp, 本程序采用TCP协议, 实现文件发送的客户端
 *  作者: ZEL
 */
#include "_public.h"


CLogFile    logfile;         // 日志文件操作类
CTcpClient  TcpClient;       // socket通信客户端类
CPActive    PActive;         // 进程心跳类


// 程序运行的参数结构体
struct st_arg {
  int  clienttype;          // 客户端类型，1-上传文件；2-下载文件
  char ip[31];              // 服务端的IP地址
  int  port;                // 服务端的端口
  int  ptype;               // 文件上传成功后文件的处理方式：1-删除文件；2-移动到备份目录
  char clientpath[301];     // 本地文件存放的根目录
  char clientpathbak[301];  // 文件成功上传后，本地文件备份的根目录，当ptype==2时有效
  bool andchild;            // 是否上传clientpath目录下各级子目录的文件，true-是；false-否
  char matchname[301];      // 待上传文件名的匹配规则，如"*.TXT,*.XML"
  char srvpath[301];        // 服务端文件存放的根目录
  int  timetvl;             // 扫描本地目录文件的时间间隔，单位：秒
  int  timeout;             // 进程心跳的超时时间
  char pname[51];           // 进程名，建议用"tcpgetfiles_后缀"的方式
} starg;


char  strsendbuffer[1024];   // 发送报文的buffer
char  strrecvbuffer[1024];   // 接收报文的buffer

bool  bcontinue = true;      // 如果调用_tcpputfile()发送了文件, bcontinue为true, 初始化为true;


// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char * strxmlbuffer);

// 心跳检测
bool ActiveTest();

// 登录业务
bool Login(const char *argv);

// 文件上传的主函数
bool _tcpputfiles();

// 把文件内容发送给对端
bool SendFile(const int sockfd, const char *filename, const int filesize);

// 删除或转存本地的文件
bool AckMessage(const char *strrecvbuffer);

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
    signal(SIGINT,EXIT); signal(SIGTERM,EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1],"a+")) {
        printf("logfile.Open(%s) failed.\n",argv[1]);
        return -1;
    }

    // 解析xml, 得到程序的运行参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg() failed.\n");
        return -1;
    }

    // 把心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout,starg.pname);

    // 向服务端发送连接请求
    if (!TcpClient.ConnectToServer(starg.ip,starg.port)) {
        printf("TcpClient.ConnectToServer(%s,%s)",starg.ip,starg.port);
        EXIT(-1);
    }
   

    // 登录业务
    if (!Login(argv[2])) {
        printf("Login(%s) failed.\n", argv[2]);
        EXIT(-1);
    }

    while (true) {
        // 调用文件上传的主函数, 执行一次文件上传的任务
        if (!_tcpputfiles()) {
            logfile.Write("_tcpputfiles() failed.\n");
            EXIT(-1);
        }

        if (bcontinue == false) {
            sleep(starg.timetvl); 

            if (!ActiveTest()) break;
        }
        
        // 更新心跳信息
        PActive.UptATime();
    }

    EXIT(0);
}










void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/tcpputfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 20 /project/tools/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>124.70.51.197</ip><port>5005</port><ptype>1</ptype><clientpath>/tmp/tcp/surdata1</clientpath><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n");
    printf("       /project/tools/bin/procctl 20 /project/tools/bin/tcpputfiles /log/idc/tcpputfiles_surfdata.log \"<ip>124.70.51.197</ip><port>5005</port><ptype>2</ptype><clientpath>/tmp/tcp/surdata1</clientpath><clientpathbak>/tmp/tcp/surfdata1bak</clientpathbak><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><srvpath>/tmp/tcp/surfdata2</srvpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpputfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是数据中心的公共功能模块，采用tcp协议把文件上传给服务端。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，如下：\n");
    printf("ip            服务端的IP地址。\n");
    printf("port          服务端的端口。\n");
    printf("ptype         文件上传成功后的处理方式：1-删除文件；2-移动到备份目录。\n");
    printf("clientpath    本地文件存放的根目录。\n");
    printf("clientpathbak 文件成功上传后，本地文件备份的根目录，当ptype==2时有效。\n");
    printf("andchild      是否上传clientpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
    printf("matchname     待上传文件名的匹配规则，如\"*.TXT,*.XML\"\n");
    printf("srvpath       服务端文件存放的根目录。\n");
    printf("timetvl       扫描本地目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
    printf("timeout       本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
    printf("pname         进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {

    ::memset(&starg,0 , sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer, "ip", starg.ip);
    if (strlen(starg.ip)==0) { 
        logfile.Write("ip is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "port", &starg.port);
    if (starg.port == 0) { 
        logfile.Write("port is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
    if ((starg.ptype != 1) && (starg.ptype != 2)) { 
        logfile.Write("ptype not in (1,2).\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
    if (strlen(starg.clientpath) == 0) { 
        logfile.Write("clientpath is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "clientpathbak", starg.clientpathbak);
    if ((starg.ptype == 2) && (strlen(starg.clientpathbak) == 0)) { 
        logfile.Write("clientpathbak is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);

    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
    if (strlen(starg.matchname) == 0) { 
        logfile.Write("matchname is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);
    if (strlen(starg.srvpath) == 0) { 
        logfile.Write("srvpath is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
    if (starg.timetvl == 0) { 
        logfile.Write("timetvl is null.\n"); 
        return false; 
    }

    // 扫描本地目录文件的时间间隔，单位：秒。
    // starg.timetvl没有必要超过30秒。
    if (starg.timetvl > 30) 
        starg.timetvl = 30;

    // 进程心跳的超时时间，一定要大于starg.timetvl，没有必要小于50秒。
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout == 0) { 
        logfile.Write("timeout is null.\n"); 
        return false; 
    }
    if (starg.timeout < 50) 
        starg.timeout=50;

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
    if (strlen(starg.pname) == 0) { 
        logfile.Write("pname is null.\n"); 
        return false;
    }

    return true;
}


bool ActiveTest() {

    // 向服务端发送心跳
    ::memset(strsendbuffer, 0, sizeof(strsendbuffer));
    ::strcpy(strsendbuffer, "<activetest>ok</activetest>");
    if (!TcpClient.Write(strsendbuffer)) return false;

    // 接收服务端的回应报文
    ::memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (!TcpClient.Read(strrecvbuffer, 20)) return false;

    return true;
}


bool Login(const char *argv) {

    // 向服务端发送登录报文
    ::memset(strsendbuffer, 0, sizeof(strsendbuffer));
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "%s<clienttype>1</clienttype>", argv);
    if (!TcpClient.Write(strsendbuffer)) return false;
    logfile.Write("发送: %s\n", strsendbuffer);

    // 接收服务端的回应报文
    ::memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (!TcpClient.Read(strrecvbuffer, 20)) return false;
    logfile.Write("接收: %s\n", strrecvbuffer);

    logfile.Write("%s:%d\n", starg.ip, starg.port);

    return true;
}


bool _tcpputfiles() {

    CDir  Dir;     // 目录操作类

    int delayed = 0;  // 未收到对端确认报文的文件数量
    int ibuflen = 0;

    bcontinue = false;

    // 打开starg.clientpath目录
    if (!Dir.OpenDir(starg.clientpath, starg.matchname, 10000, starg.andchild)) {
        logfile.Write("Dir.OpenDir(%s) failed.\n", starg.clientpath);
        return false;
    }

    while (true) {
        ::memset(strsendbuffer, 0, sizeof(strsendbuffer));
        ::memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

        // 遍历目录中的文件, 调用ReadDir()获取一个文件名
        if (!Dir.ReadDir()) break;

        bcontinue = true;

        // 把文件名、修改时间、文件大小组成报文, 发送给服务端
        SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><mtime>%s<mtime><size>%d</size>", Dir.m_FullFileName, Dir.m_ModifyTime, Dir.m_FileSize);
        if (!TcpClient.Write(strsendbuffer)) {
            logfile.Write("TcpClient.Write() failed.\n");
            return false;
        }
        
        // 把文件的内容发送个服务端
        logfile.Write("send %s(%d) ...", Dir.m_FullFileName, Dir.m_FileSize);
        if (SendFile(TcpClient.m_connfd, Dir.m_FullFileName, Dir.m_FileSize)) {
            logfile.WriteEx(" ok.\n");
            delayed++;
        } else {
            logfile.WriteEx(" failed.\n");
            TcpClient.Close();
            return false;
        }

        // 更新心跳信息
        PActive.UptATime();

        // 接收对端的确认报文
        while (delayed > 0) {
            ::memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
            if (!TcpRead(TcpClient.m_connfd, strrecvbuffer, &ibuflen, -1)) break;
            delayed--;

            // 删除或者转存到备份目录
            AckMessage(strrecvbuffer);
        }

        
    }

    // 如果没有接收完, 继续接收
    while (delayed > 0) {
        if (!TcpRead(TcpClient.m_connfd, strrecvbuffer, &ibuflen, 10)) break;
        delayed--;

        // 删除或者转存到备份目录
        AckMessage(strrecvbuffer);
    }


    return true;
}


bool SendFile(const int sockfd, const char *filename, const int filesize) {

    int  onread = 0;        // 每次调用fread时打算读取的字节数
    int  bytes = 0;         // 调用一次fread从文件中读取的字节数
    char buffer[1000];      // 存放读取数据的buffer
    int  totalbytes = 0;    // 从文件中已读取的字节总数
    FILE *fp = NULL;        // 文件指针

    // 以“rb”的模式发开文件
    if ((fp = fopen(filename,"rb")) == NULL) return false;

    while (true) {
        ::memset(buffer, 0, sizeof(buffer));

        // 计算本次应该读取的字节数, 如果剩余的数据超过1000字节, 就打算读取1000字节
        if ((filesize - totalbytes) > 1000) 
            onread = 1000;
        else
            onread = filesize - totalbytes;

        // 从文件中读取数据
        bytes = fread(buffer, 1, onread,fp);

        // 把读取到的数据发送给对端
        if (bytes > 0) {
            if (!Writen(sockfd, buffer, bytes)) {
                fclose(fp);
                return false;
            }
        }

        // 计算文件已读取的字节总数, 如果文件已读完, 跳出循环
        totalbytes += bytes;
        if (totalbytes == filesize) break;
    }

    // 关闭文件指针
    fclose(fp);

    return true;
}


bool AckMessage(const char *strrecvbuffer) {

    // 解析strrecvbuffer, 得到文件名
    char filename[301];   ::memset(filename, 0, sizeof(filename));
    char result[11];      ::memset(result, 0, sizeof(result));
    GetXMLBuffer(strrecvbuffer, "filename", filename, 300);
    GetXMLBuffer(strrecvbuffer, "result", result, 10);
    
    // 如果服务端接收文件不成功, 直接返回
    if (strcmp(result ,"ok") != 0) return false;

    // 如果ptype=1, 删除文件
    if (starg.ptype == 1) {
        if (!REMOVE(filename)) {
            logfile.Write("REMOVE(%s) failed.\n", filename);
            return false;
        }
    }

    // 如果ptype=2, 移动到备份目录
    if (starg.ptype == 2) {
        // 生成备份目录文件名
        char bakfilename[301]; ::memset(bakfilename, 0, sizeof(bakfilename));
        ::strcpy(bakfilename, filename);
        UpdateStr(bakfilename, starg.clientpath, starg.clientpathbak, false);

        // 备份文件
        if (!RENAME(filename, bakfilename)) {
            logfile.Write("RENAME(%s,%s) failed.\n", filename, bakfilename);
            return false;
        }
    }

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}