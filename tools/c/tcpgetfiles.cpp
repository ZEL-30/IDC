/*
 *  程序名: tcpgetfiles.cpp, 本程序采用tcp协议，实现文件下载的客户端
 *  作者: ZEL
 */
#include "_public.h"


CLogFile     logfile;       // 日志文件操作类
CTcpClient   TcpClient;     // socket通信客户端类
CPActive     PActive;       // 进程心跳操作类


// 程序运行的参数结构体
struct st_arg
{
  int  clienttype;          // 客户端类型，1-上传文件；2-下载文件。
  char ip[31];              // 服务端的IP地址。
  int  port;                // 服务端的端口。
  int  ptype;               // 文件下载成功后服务端文件的处理方式：1-删除文件；2-移动到备份目录。
  char srvpath[301];        // 服务端文件存放的根目录。
  char srvpathbak[301];     // 文件成功下载后，服务端文件备份的根目录，当ptype==2时有效。
  bool andchild;            // 是否下载srvpath目录下各级子目录的文件，true-是；false-否。
  char matchname[301];      // 待下载文件名的匹配规则，如"*.TXT,*.XML"。
  char clientpath[301];     // 客户端文件存放的根目录。
  int  timetvl;             // 扫描服务端目录文件的时间间隔，单位：秒。
  int  timeout;             // 进程心跳的超时时间。
  char pname[51];           // 进程名，建议用"tcpgetfiles_后缀"的方式。
} starg;


char  strsendbuffer[1024];   // 发送报文的buffer
char  strrecvbuffer[1024];   // 接收报文的buffer



// 程序帮助文档
void _help();

// 解析XML, 得到程序运行参数
bool _xmltoarg(const char *strxmlbuffer);

// 登录业务
bool Login(const char *arg);

// 文件下载主函数
void _tcpgetfiles();

// 下载文件内容
bool RecvFile(const int sockfd, const char *filename, const char *mtime, const int filesize);

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

    // 解析xml, 得到程序的运行参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg(%s) failed.\n", argv[2]);
        return -1;
    }

    // 把心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    // 连接TCP服务端
    if (!TcpClient.ConnectToServer(starg.ip, starg.port)) {
        logfile.Write("TcpClient.ConnectToServer(%s, %d) failed.\n", starg.ip, starg.port);
        EXIT(-1);
    }

    // 登录业务
    if (!Login(argv[2])) {
        logfile.Write("Login(%s) failed.\n", argv[2]);
        EXIT(-1);
    }

    // 调用文件下载主函数
    _tcpgetfiles();

    EXIT(0);
}











void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/tcpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 20 /project/tools/bin/tcpgetfiles /log/idc/tcpgetfiles_surfdata.log \"<ip>124.70.51.197</ip><port>5005</port><ptype>1</ptype><srvpath>/tmp/tcp/surfdata2</srvpath><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><clientpath>/tmp/tcp/surfdata3</clientpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>\"\n");
    printf("       /project/tools/bin/procctl 20 /project/tools/bin/tcpgetfiles /log/idc/tcpgetfiles_surfdata.log \"<ip>124.70.51.197</ip><port>5005</port><ptype>2</ptype><srvpath>/tmp/tcp/surfdata2</srvpath><srvpathbak>/tmp/tcp/surfdata2bak</srvpathbak><andchild>true</andchild><matchname>*.XML,*.CSV,*.JSON</matchname><clientpath>/tmp/tcp/surfdata3</clientpath><timetvl>10</timetvl><timeout>50</timeout><pname>tcpgetfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是数据中心的公共功能模块，采用tcp协议从服务端下载文件。\n");
    printf("logfilename   本程序运行的日志文件。\n");
    printf("xmlbuffer     本程序运行的参数，如下：\n");
    printf("ip            服务端的IP地址。\n");
    printf("port          服务端的端口。\n");
    printf("ptype         文件下载成功后服务端文件的处理方式：1-删除文件；2-移动到备份目录。\n");
    printf("srvpath       服务端文件存放的根目录。\n");
    printf("srvpathbak    文件成功下载后，服务端文件备份的根目录，当ptype==2时有效。\n");
    printf("andchild      是否下载srvpath目录下各级子目录的文件，true-是；false-否，缺省为false。\n");
    printf("matchname     待下载文件名的匹配规则，如\"*.TXT,*.XML\"\n");
    printf("clientpath    客户端文件存放的根目录。\n");
    printf("timetvl       扫描服务目录文件的时间间隔，单位：秒，取值在1-30之间。\n");
    printf("timeout       本程序的超时时间，单位：秒，视文件大小和网络带宽而定，建议设置50以上。\n");
    printf("pname         进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {

    memset(&starg,0,sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer, "ip", starg.ip);
    if (strlen(starg.ip) == 0) {
        logfile.Write("ip is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "port", &starg.port);
    if ( starg.port == 0) {
        logfile.Write("port is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
    if ((starg.ptype != 1) && (starg.ptype != 2)) {
        logfile.Write("ptype not in (1,2).\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);
    if (strlen(starg.srvpath) == 0) {
        logfile.Write("srvpath is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "srvpathbak", starg.srvpathbak);
    if ((starg.ptype == 2) && (strlen(starg.srvpathbak) == 0)) {
        logfile.Write("srvpathbak is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);

    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
    if (strlen(starg.matchname) == 0) {
        logfile.Write("matchname is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
    if (strlen(starg.clientpath) == 0) {
        logfile.Write("clientpath is null.\n"); 
        return false; 
    }

    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
    if (starg.timetvl == 0) {
        logfile.Write("timetvl is null.\n"); 
        return false; 
    }

    // 扫描服务端目录文件的时间间隔，单位：秒。
    // starg.timetvl没有必要超过30秒。
    if (starg.timetvl > 30) starg.timetvl = 30;

    // 进程心跳的超时时间，一定要大于starg.timetvl，没有必要小于50秒。
    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout == 0) {
        logfile.Write("timeout is null.\n"); 
        return false; 
    }
    if (starg.timeout < 50) starg.timeout = 50;

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname,50);
    if (strlen(starg.pname) == 0) {
        logfile.Write("pname is null.\n"); 
        return false; 
    }

    return true;
}


bool Login(const char *arg) {

    // 向服务端发送登录请求
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "%s<clienttype>2</clienttype>", arg);
    if (!TcpClient.Write(strsendbuffer)) {
        logfile.Write("TcpClient.Write(%s) failed.\n", strsendbuffer);
        return false;
    }
    logfile.Write("发送: %s\n", strsendbuffer);

    // 接收服务端的回应报文
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (!TcpClient.Read(strrecvbuffer, 20)) {
        logfile.Write("TcpClient.Read(%s) failed.\n", strrecvbuffer);
        return false;
    }
    logfile.Write("接收: %s\n", strrecvbuffer);

    logfile.Write("%s:%d\n", starg.ip, starg.port);

    return true;
}


void _tcpgetfiles() {

    while (true) {
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        memset(strsendbuffer, 0, sizeof(strsendbuffer));

        // 接收服务端发送的报文
        if (!TcpClient.Read(strrecvbuffer, starg.timetvl + 10)) {
            logfile.Write("TcpClient.Read() failed.\n");
            return ;
        }
       
        // 处理心跳报文
        if (strcmp(strrecvbuffer, "<activetest>ok</activetest>") == 0) {
            strcpy(strsendbuffer, "ok");
            if (TcpClient.Write(strsendbuffer) == false) {
                logfile.Write("TcpClient.Write() failed.\n"); return;
            }
        }

        // 处理下载文件的报文
        if (strncmp(strrecvbuffer, "<filename>", 10) == 0) {
            char  serverfilename[301];    memset(serverfilename, 0, sizeof(serverfilename));
            char  mtime[21];              memset(mtime, 0, sizeof(mtime));
            int   filesize = 0;

            // 解析文件名、文件时间和文件大小
            GetXMLBuffer(strrecvbuffer, "filename", serverfilename, 300);
            GetXMLBuffer(strrecvbuffer, "mtime", mtime, 20);
            GetXMLBuffer(strrecvbuffer, "size", &filesize);

            // 客户端和服务端文件的目录是不一样的, 以下代码生成客户端的文件名
            // 把文件名中的srvpath替换成clientpath, 要小心第三个参数
            char  clientfilename[301]; memset(clientfilename, 0, sizeof(clientfilename));
            STRCPY(clientfilename, sizeof(serverfilename), serverfilename);
            UpdateStr(clientfilename, starg.srvpath, starg.clientpath, false);

            // 接收对端的文件内容
            logfile.Write("recv %s(%d) ...", clientfilename, filesize);
            if (RecvFile(TcpClient.m_connfd, clientfilename, mtime, filesize)) {
                logfile.WriteEx(" ok.\n");
            } else {
                logfile.WriteEx(" failed.\n");
                TcpClient.Close();
                return ;
            }

            // 把接收结果发送给对端
            SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>ok<result>", serverfilename);
            if (!TcpClient.Write(strsendbuffer)) {
                logfile.Write("TcpClient.Write(%s) failed.\n", strsendbuffer);
                return ;
            }
        }
    }
}


bool RecvFile(const int sockfd, const char *filename, const char *mtime, const int filesize) {

    int  onwrite = 0;     // 本次打算接收的字节数
    int  totalbytes = 0;  // 以成功接收的字节总数
    char buffer[1000];    // 写入数据的buffer
    FILE *fp = NULL;      // 文件指针
    
    // 生成临时文件
    char strfilenametmp[301]; memset(strfilenametmp, 0, sizeof(strfilenametmp));
    SPRINTF(strfilenametmp, sizeof(strfilenametmp), "%s.tmp", filename);
    
    // 打开临时文件
    if ((fp = FOPEN(strfilenametmp, "wb")) == NULL) return false;

    // 接收对端发送的文件内容
    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // 计算本次应该接受的字节数
        if ((filesize - totalbytes) > 1000)
            onwrite = 1000;
        else
            onwrite = filesize - totalbytes;


        // 接收服务端发送的文件内容
        if (!Readn(sockfd, buffer, onwrite)) {
            fclose(fp);
            return false;
        }

        // 把接收的文件内容写入文件
        fwrite(buffer, 1, onwrite, fp);

        // 计算已接收文件的总字节数
        totalbytes += onwrite;

        // 如果文件接收完, 跳出循环
        if (totalbytes == filesize) break;
    }

    // 关闭临时文件
    fclose(fp);

    // 修改文件时间
    UTime(strfilenametmp, mtime);

    // 把临时文件名改名为正式文件名
    if (!RENAME(strfilenametmp, filename)) return false;

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n", sig);
    exit(0);
}