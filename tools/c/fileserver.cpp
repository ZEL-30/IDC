/*
 *  程序名: fileserver.cpp, 本程序采用TCP协议, 实现文件传输的服务端
 *  作者: ZEL
 */
#include "_public.h"


CLogFile    logfile;          // 日志文件操作类
CTcpServer  TcpServer;        // socket通信的服务端
CPActive    PActive;          // 进程心跳操作类


// 程序运行的参数结构体
struct st_arg {
    int  clienttype;          // 客户端类型，1-上传文件；2-下载文件
    char ip[31];              // 服务端的IP地址
    int  port;                // 服务端的端口
    int  ptype;               // 文件成功传输后的处理方式：1-删除文件；2-移动到备份目录。
    char clientpath[301];     // 客户端文件存放的根目录
    bool andchild;            // 是否传输各级子目录的文件，true-是；false-否
    char matchname[301];      // 待传输文件名的匹配规则，如"*.TXT,*.XML"
    char srvpath[301];        // 服务端文件存放的根目录
    char srvpathbak[301];     // 服务端文件存放的根目录
    int  timetvl;             // 扫描目录文件的时间间隔，单位：秒
    int  timeout;             // 进程心跳的超时时间
    char pname[51];           // 进程名，建议用"tcpgetfiles_后缀"的方式
} starg;


char  strsendbuffer[1024];    // 发送报文的buffer
char  strrecvbuffer[1024];    // 接收报文的buffer

bool  bcontinue = true;      // 如果调用SendFileMain()发送了文件, bcontinue为true, 初始化为true;


// 程序帮助文档
void _help();

// 把XML解析到参数starg结构体中
bool _xmltoarg(const char * strxmlbuffer);

// 心跳检测
bool ActiveTest();

// 登录业务处理函数
bool ClientLogin();

// 上传文件的主函数
void RecvFileMain();

// 接收文件内容
bool RecvFile(const int sockfd, const char *filename, const char *mtime,const int filesize);

// 下载文件的主函数
void SendFileMain();

// 发送文件内容
bool SendFile(const int sockfd, const char *filename, const int filesize);

// 删除或转存到备份目录
bool AckMessage(const char *strrecvbuffer);

// 父进程退出和信号2,15的处理函数
void FathEXIT(int sig);

// 子进程退出和信号2,15的处理函数
void ChldEXIT(int sig);




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
    signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);
    

    // 打开日志文件
    if (!logfile.Open(argv[2],"a+")) {
        printf("logfile.Open(%s) failed.\n", argv[2]);
        return -1;
    }

    while (true) {

        // 服务端初始化
        if (!TcpServer.InitServer(atoi(argv[1]))) {
            logfile.Write("TcpClient.InitServerr(%s) failed.\n", argv[1]);
            return -1;
        }

        // 等待客户端的连接请求
        if (!TcpServer.Accept()) {
            logfile.Write("TcpServer.Accept() failed.\n");
            return -1;
        }


        // 创建子进程, 如果是父进程, 关闭客户端的socket, 然后continue继续监听客户端请求
        if (fork() > 0) {
            // 关闭用于监听的socket
            TcpServer.CloseClient();
            continue;
        }

        // 子进程与客户端进行通讯, 处理业务
        // 子进程设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
        // 但请不要用 "kill -9 + 进程号" 强行终止
        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);

        // 子进程关闭监听socket   
        TcpServer.CloseListen();

        
        // 处理登录客户端的登录报文
        if (!ClientLogin()) ChldEXIT(-1);

        // 如果clienttype=1, 调用上传文件的主函数
        if (starg.clienttype == 1) RecvFileMain();

        // 如果clienttype=2, 调用下载文件的主函数
        if (starg.clienttype == 2) SendFileMain();



        // 子进程退出
        ChldEXIT(0);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./fileserver port logfile\n");
    printf("Example:./fileserver.out 5005 /log/idc/fileserver.log\n"); 
    printf("         /project/tools/bin/procctl 10 /project/tools/bin/fileserver 5005 /log/idc/fileserver.log\n\n\n"); 
}


bool _xmltoarg(const char * strxmlbuffer) {

    memset(&starg,0 , sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer,"clienttype",&starg.clienttype);
    GetXMLBuffer(strxmlbuffer, "ip", starg.ip);
    GetXMLBuffer(strxmlbuffer, "port", &starg.port);
    GetXMLBuffer(strxmlbuffer, "ptype", &starg.ptype);
    GetXMLBuffer(strxmlbuffer, "clientpath", starg.clientpath);
    GetXMLBuffer(strxmlbuffer, "andchild", &starg.andchild);
    GetXMLBuffer(strxmlbuffer, "matchname", starg.matchname);
    GetXMLBuffer(strxmlbuffer, "srvpath", starg.srvpath);

    GetXMLBuffer(strxmlbuffer, "timetvl", &starg.timetvl);
    if (starg.timetvl > 30) 
        starg.timetvl = 30;

    GetXMLBuffer(strxmlbuffer, "timeout", &starg.timeout);
    if (starg.timeout < 50) 
        starg.timeout=50;

    GetXMLBuffer(strxmlbuffer, "pname", starg.pname, 50);
    strcat(starg.pname, "_srv");

    return true;
}


bool ActiveTest() {
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

    // 向服务端发送请求报文
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<activetest>ok</activetest>");
    if (TcpServer.Write(strsendbuffer) == false) return false; 

    // 接收服务端的回应报文
    if (TcpServer.Read(strrecvbuffer,20) == false) return false; 

    return true;
}


bool ClientLogin() {
    
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    memset(strsendbuffer, 0, sizeof(strsendbuffer));

    // 接收客户端的登录报文
    if (!TcpServer.Read(strrecvbuffer, 20)) {
        logfile.Write("TcpServer.Read() failed.\n");
        return false;
    }
    logfile.Write("strrecvbuffer = %s.\n", strrecvbuffer);

    // 解析登录报文
    _xmltoarg(strrecvbuffer);

    // 判断客户端的合法性
    if (starg.clienttype != 1 && starg.clienttype != 2) {
        strcpy(strsendbuffer, "failed");
    } else {
        strcpy(strsendbuffer, "ok");
    }

    // 向客户端发送回应报文
    if (!TcpServer.Write(strsendbuffer)) {
        logfile.Write("TcpServer.Write() failed.\n");
        return false;
    }

    logfile.Write("%s login %s.\n", TcpServer.GetIP(), strrecvbuffer);

    return true;    
}


void RecvFileMain() {

    // 把进程的信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    while (true) {
        memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
        memset(strsendbuffer, 0, sizeof(strsendbuffer));

        // 更新心跳信息
        PActive.UptATime();

        // 接收客户端的报文
        if (!TcpServer.Read(strrecvbuffer, starg.timetvl + 10)) {
            logfile.Write("TcpServer.Read() failed.\n");
            return ;
        }

        // 处理心跳报文
        if (strcmp(strrecvbuffer, "<activetest>ok</activetest>") == 0) {
            strcpy(strsendbuffer, "ok");
            if (!TcpServer.Write(strsendbuffer)) break;
        }

        // 处理上传文件的请求报文
        if (strncmp(strrecvbuffer, "<filename>", 10) == 0) {
            char clientfilename[301];  // 文件名
            char mtime[21];            // 文件时间
            int  filesize;             // 文件大小

            // 解析文件名、文件时间、文件大小
            memset(clientfilename, 0, sizeof(clientfilename));
            GetXMLBuffer(strrecvbuffer, "filename", clientfilename, 300);
            memset(mtime, 0, sizeof(mtime));
            GetXMLBuffer(strrecvbuffer, "mtime", mtime, 20);
            GetXMLBuffer(strrecvbuffer, "size", &filesize);

            // 客户端和服务端文件的目录是不一样的, 以下代码生成服务端的文件名
            // 把文件名中的clientpath替换成srvpath, 要小心第三个参数
            char serverfilename[301]; memset(serverfilename, 0, sizeof(serverfilename));
            strcpy(serverfilename, clientfilename);
            UpdateStr(serverfilename, starg.clientpath, starg.srvpath, false);

            // 接收上传的文件内容
            logfile.Write("recv %s(%d) ...", serverfilename, filesize);
            if (RecvFile(TcpServer.m_connfd, serverfilename, mtime,filesize)) {
                logfile.WriteEx(" ok.\n");
            } else {
                logfile.WriteEx(" failed.\n");
                return ;
            }

            // 把接收结果返回给对端
            SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><result>ok</result>", clientfilename);
            if (!TcpServer.Write(strsendbuffer)) {
                logfile.Write("TcpServer.Write() failed.\n");
                return ;
            }
        }

    }


}


bool RecvFile(const int sockfd, const char *filename, const char *mtime,const int filesize) {

    int  onwrite = 0;       // 本次打算接收的字节数
    int  totalbytes = 0;    // 已接收文件的总字节数
    char buffer[1000];      // 接收文件内容的缓冲区
    FILE *fp = NULL;        // 文件指针

    // 生成临时文件名
    char strfilenametmp[301]; memset(strfilenametmp, 0, sizeof(strfilenametmp));
    strcpy(strfilenametmp, filename);
    strcat(strfilenametmp, ".tmp");

    // 以写入的方式打开临时文件
    if ((fp = FOPEN(strfilenametmp, "wb")) == NULL) return false;

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // 计算本次应该接收的字节数
        if ((filesize - totalbytes) > 1000)
            onwrite = 1000;
        else
            onwrite = filesize - totalbytes;

        // 接收客户端发送的文件内容
        if (!Readn(sockfd, buffer, onwrite)) {
            fclose(fp);
            return false;
        }
        
        // 把接收到的文件内容写入临时文件
        fwrite(buffer, 1, onwrite, fp);

        // 计算已接收文件的总字节数
        totalbytes += onwrite;

        // 如果文件接收完, 跳出循环
        if (totalbytes == filesize) break;
    }

    // 关闭临时文件
    fclose(fp);

    // 重置文件的时间
    UTime(strfilenametmp,mtime);

    // 把临时文件改命为正式文件名
    if (!RENAME(strfilenametmp, filename, 2)) return false;

    return true;
}


void SendFileMain() {
    // 把进程的信息写入共享内存
    PActive.AddPInfo(starg.timeout, starg.pname);

    while (true) {
        CDir  Dir;   // 目录操作类
    
        int  delayed = 0;   // 未收到对端确认报文的数量
        int  ibuflen = 0;
            
        bcontinue = false;

        // 打开srvpath目录
        if (!Dir.OpenDir(starg.srvpath, starg.matchname, 10000, starg.andchild)) {
            logfile.Write("Dor.OpenDir(%s) failed.\n", starg.srvpath);
            return ;
        }

        while (true) {
            memset(strsendbuffer, 0, sizeof(strsendbuffer));
            memset(strrecvbuffer, 0, sizeof(strrecvbuffer));

            // 读取每个文件的信息
            if (!Dir.ReadDir()) break;

            // 把文件名、修改时间、文件大小组成报文, 发送给客户端
            SNPRINTF(strsendbuffer, sizeof(strsendbuffer), 1000, "<filename>%s</filename><mtime>%s</mtime><size>%d</size>", Dir.m_FullFileName, Dir.m_ModifyTime, Dir.m_FileSize);
            if (!TcpServer.Write(strsendbuffer)) {
                logfile.Write("TcpServer.Write() failed.\n");
                return ;
            }

            // 发送文件内容
            logfile.Write("send %s(%d) ...", Dir.m_FullFileName, Dir.m_FileSize);
            if (SendFile(TcpServer.m_connfd, Dir.m_FullFileName, Dir.m_FileSize)){
                logfile.WriteEx(" ok.\n");
                delayed++;
            } else {
                logfile.WriteEx(" failed.\n");
                TcpServer.CloseClient();
                return ;
            }

            while (delayed > 0) {
                // 等待对端的确认报文
                memset(strrecvbuffer,0,sizeof(strrecvbuffer));
                if (!TcpRead(TcpServer.m_connfd, strrecvbuffer, &ibuflen, -1)) break;
                delayed--;

                // 删除或转存到备份目录
                AckMessage(strrecvbuffer);
            }
        }

        while (delayed > 0) {
            // 等待对端的确认报文
            memset(strrecvbuffer,0,sizeof(strrecvbuffer));
            if (!TcpRead(TcpServer.m_connfd, strrecvbuffer, &ibuflen, 10)) break;
            delayed--;

            // 删除或转存到备份目录
            AckMessage(strrecvbuffer);
        }

        if (bcontinue==false) {
            sleep(starg.timetvl);

            if (!ActiveTest()) break;
        }

        PActive.UptATime();
    }

}


bool SendFile(const int sockfd, const char *filename, const int filesize) {

    int  onread = 0;       // 本次打算发送的字节数
    int  bytes = 0;        // 本次成功发送的字节数
    int  totalbytes = 0;   // 已成功发送的字节总数
    char buffer[1000];     // 存放读取数据的buffer
    FILE *fp = NULL;       // 文件指针

    // 以rb的方式打开文件
    if ((fp = fopen(filename, "rb")) == NULL) {
        logfile.Write("fopen(%s) failed.\n", filename);
        return false;
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        // 计算本次应该读取的字节数, 如果剩余的数据超过1000字节, 就打算读取1000字节
        if ((filesize - totalbytes) > 1000)
            onread = 1000;
        else
            onread = filesize - totalbytes;

        // 读取文件内容
        bytes = fread(buffer, 1, onread, fp);

        // 把读取到的内容发送给对端
        if (bytes > 0) {
            if (!Writen(sockfd, buffer, bytes)) {
                fclose(fp);
                return false;
            }
        }

        // 计算文件已读取的字节数
        totalbytes += bytes;

        // 如果文件已读取完成, 跳出循环
        if (filesize == totalbytes) break;
    }

    // 关闭文件指针
    fclose(fp);

    return true;
}


bool AckMessage(const char *strrecvbuffer){

    char filename[301]; memset(filename, 0, sizeof(filename));
    char result[31];       memset(result, 0, sizeof(result));

    // 解析strrecvbuffer, 得到文件名
    GetXMLBuffer(strrecvbuffer, "filename", filename, 300);
    GetXMLBuffer(strrecvbuffer, "result", result, 30);

    if (strcmp(result, "ok") == 0) return false;

    
    // 如果ptype=1, 删除文件
    if (starg.ptype == 1) {
        if (!REMOVE(filename)) {
            logfile.Write("REMOVE(%s) failed.\n", filename);
            return false;
        }
    }

    // 如果ptype=2, 转存到备份目录
    if (starg.ptype == 2) {
        // 生成备份目录
        char bakfilename[301]; memset(bakfilename, 0, sizeof(bakfilename));
        STRCPY(bakfilename, sizeof(bakfilename), filename);
        UpdateStr(bakfilename, starg.srvpath, starg.srvpath, false);

        if (!RENAME(filename, bakfilename,2)) {
            logfile.Write("RENAME(%s,%s) failed.\n", filename, bakfilename);
            return false;
        }
    }

    return true;
}


void FathEXIT(int sig) {

    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    // 父进程关闭监听socket
    TcpServer.CloseListen();

    // 父进程通知所有子进程退出
    kill(0, SIGTERM);

    // 父进程退出
    logfile.Write("父进程退出, sig = %d.\n", sig);
    exit(0);
}


void ChldEXIT(int sig) {

    // 以下代码是为了防止信号处理函数在执行过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    // 子进程关闭客户端socket
    TcpServer.CloseClient();

    // 子进程退出
    logfile.Write("子进程退出, sig = %d.\n", sig);
    exit(0);
}


