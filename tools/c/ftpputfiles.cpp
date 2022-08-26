/*
 *  程序名: ftpputfiles.cpp, 本程序
 *  作者: ZEL
/project/tools/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log "<host>127.0.0.1:21</host><mode>1</mode><username>zel</username><password>19981110</password><localpath>/tmp/client</localpath><remotepath>/tmp/server</remotepath><matchname>*.txt</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>"
 */

#include "_public.h"
#include "_ftp.h"


CLogFile  logfile;       // 日志文件操作类
Cftp      ftp;           // FTP操作类
CPActive  PActive;       // 进程心跳类


// 程序运行参数的结构体。
struct st_arg
{
  char host[31];           // 远程服务端的IP和端口。
  int  mode;               // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。
  char username[31];       // 远程服务端ftp的用户名。
  char password[31];       // 远程服务端ftp的密码。
  char remotepath[301];    // 远程服务端存放文件的目录。
  char localpath[301];     // 本地文件存放的目录。
  char matchname[101];     // 待上传文件匹配的规则。
  int  ptype;              // 上传后客户端文件的处理方式：1-什么也不做；2-删除；3-备份。
  char localpathbak[301];  // 上传后客户端文件的备份目录。
  char okfilename[301];    // 已上传成功文件名清单。
  int  timeout;            // 进程心跳的超时时间。
  char pname[51];          // 进程名，建议用"ftpputfiles_后缀"的方式。
} starg;

// 文件信息的结构体
struct st_fileinfo {
    char  filename[301];    // 文件名
    char  mtime[31];        // 文件时间
};


vector<struct st_fileinfo> vlistfile1;  // 已成功上传的文件名的容器, 从okfilename中加载
vector<struct st_fileinfo> vlistfile2;  // 上传前列出客户端文件名的容器
vector<struct st_fileinfo> vlistfile3;  // 本次不需要上传的文件名的容器
vector<struct st_fileinfo> vlistfile4;  // 本次需要上传的文件名的容器


// 程序帮助文档
void _help();

// 把xml解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 文件上传主函数
bool _ftpputfiles();

// 加载本地文件
bool LoadLocalFile();

// 加载okfilename文件中的内容到容器vlistfile1中
bool LoadOKFile();

// 比较vlistfile1和vlistfile2, 得到vlistfile3和vlistfile4
bool CompVector();

// 把容器vlistfile3中的内容写入okfilename文件, 覆盖之前的okfilename文件
bool WriteToOKFile();

// 把上传成功的文件记录追加到okfilename文件中
bool AppendToOKFile(struct st_fileinfo *stfileinfo);

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
    CloseIOAndSignal(true);
    signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

    // 打开日志文件
    if (!logfile.Open(argv[1],"a+")) {
        printf("logfile.Open(%s) failed.\n",argv[1]);
        return -1;
    }

    // 解析xml, 得到程序的运行参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg() failed.\n");
        EXIT(-1);
    }

    // 把心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout,starg.pname);

    // 登录FTP服务器
    if (!ftp.login(starg.host,starg.username,starg.password,starg.mode)) {
        logfile.Write("ftp.login(%s,%s,%s) failed.\n",starg.host,starg.username,starg.password);
        EXIT(-1);
    }

    // 文件上传功能主函数
    _ftpputfiles();

    // 注销登录
    ftp.logout();

    return 0;
}









void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/ftpputfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpputfiles /log/idc/ftpputfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>zel</username><password>19981110</password><localpath>/tmp/idc/surfdata</localpath><remotepath>/tmp/ftpputest</remotepath><matchname>SURF_ZH*.JSON</matchname><ptype>1</ptype><localpathbak>/tmp/idc/surfdatabak</localpathbak><okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename><timeout>80</timeout><pname>ftpputfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把本地目录中的文件上传到远程的ftp服务器。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件上传的参数，如下：\n");
    printf("<host>127.0.0.1:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>zel</username> 远程服务端ftp的用户名。\n");
    printf("<password>19981110</password> 远程服务端ftp的密码。\n");
    printf("<remotepath>/tmp/ftpputest</remotepath> 远程服务端存放文件的目录。\n");
    printf("<localpath>/tmp/idc/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.JSON</matchname> 待上传文件匹配的规则。"\
            "不匹配的文件不会被上传，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<ptype>1</ptype> 文件上传成功后，本地文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<localpathbak>/tmp/idc/surfdatabak</localpathbak> 文件上传成功后，本地文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("<okfilename>/idcdata/ftplist/ftpputfiles_surfdata.xml</okfilename> 已上传成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("<timeout>80</timeout> 上传文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("<pname>ftpputfiles_surfdata</pname> 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}


bool _xmltoarg(const char *strxmlbuffer) {

    memset(&starg, 0, sizeof(struct st_arg));

    GetXMLBuffer(strxmlbuffer,"host",starg.host,30);
    if (strlen(starg.host) == 0) {
        logfile.Write("host is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"mode",&starg.mode);
    if (starg.mode != 2) {
        starg.mode = 1;
    }

    GetXMLBuffer(strxmlbuffer,"username",starg.username,30);
    if (strlen(starg.username) == 0) {
        logfile.Write("username is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"password",starg.password,30);
    if (strlen(starg.password) == 0) {
        logfile.Write("password is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"remotepath",starg.remotepath,300);
    if (strlen(starg.remotepath) == 0) {
        logfile.Write("remotepath is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"localpath",starg.localpath,300);
    if (strlen(starg.localpath) == 0) {
        logfile.Write("localpath is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"matchname",starg.matchname,100);
    if (strlen(starg.matchname) == 0) {
        logfile.Write("matchname is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
    if (starg.ptype != 1 && starg.ptype != 2 && starg.ptype != 3) {
        logfile.Write("ptype is error.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"localpathbak",starg.localpathbak,300);
    if (starg.ptype == 3 && strlen(starg.localpathbak) == 0) {
        logfile.Write("remotepathbak is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename,300);
    if (starg.ptype == 1 && strlen(starg.okfilename) == 0) {
        logfile.Write("okfilename is null.\n");
        return false;
    }
    
    GetXMLBuffer(strxmlbuffer,"timeout",&starg.timeout);
    if (starg.timeout == 0) {
        logfile.Write("timeout is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"pname",starg.pname,50);
    if (strlen(starg.pname) == 0) {
        logfile.Write("pname is null.\n");
        return false;
    }

    return true;
}


bool _ftpputfiles() {

    // 把localpath目录下的文件加载到vlistfile2容器中。
    if (!LoadLocalFile()) {
        logfile.Write("LoadLocalFile() failed.\n");
        return false;
    }

    // 更新程序的心跳
    PActive.UptATime();

    if (starg.ptype == 1) {

        // 加载okfilename文件中的内容到容器vlistfile1中
        LoadOKFile();

        // 比较vlistfile1和vlistfile2, 得到vlistfile3和vlistfile4
        CompVector();

        // 把容器vlistfile3中的内容写入okfilename文件, 覆盖之前的okfilename文件
        WriteToOKFile();

        // 把vlistfile4中的内容复制到vlistfile2中
        vlistfile2.clear();
        vlistfile2.swap(vlistfile4);
    }

    // 更新程序的心跳
    PActive.UptATime();

    char strlocalfilename[301];  char strremotefilename[301];

    // 遍历vlistfile2容器
    for (int i = 0;i < vlistfile2.size();i++) {
        memset(strlocalfilename,0,sizeof(strlocalfilename));
        memset(strremotefilename,0,sizeof(strremotefilename));

        sprintf(strlocalfilename,"%s/%s",starg.localpath,vlistfile2[i].filename);
        sprintf(strremotefilename,"%s/%s",starg.remotepath,vlistfile2[i].filename);

        // 开始上传文件
        logfile.Write("put %s ...",strlocalfilename);

        // 更新程序的心跳
        PActive.UptATime();

        // 调用ftp.put()方法上传文件到服务器
        if (!ftp.put(strlocalfilename,strremotefilename,true)) {
            logfile.WriteEx(" failed.\n");
            return false;
        }   

        logfile.WriteEx(" ok.\n");

        // 如果ptype=1,把上传成功的文件记录追加到okfilename文件中
        if (starg.ptype == 1)
            AppendToOKFile(&vlistfile2[i]);

        // 如果ptype=2,删除客户端上的文件
        if (starg.ptype == 2) {
            if (!REMOVE(strlocalfilename)) {
                logfile.Write("REMOVE(%s) failed.\n",strlocalfilename);
                return false;
            }
        }

        // 如果ptype=3,备份客户端上的文件
        if (starg.ptype == 3) {
            char strlocalfilenamebak[301];     // 本地备份文件名
            memset(strlocalfilenamebak,0,sizeof(strlocalfilenamebak));
            sprintf(strlocalfilenamebak,"%s/%s",starg.localpathbak,vlistfile2[i].filename);
            if (!RENAME(strlocalfilename,strlocalfilenamebak)) {
                logfile.Write("RENAME(%s,%s) failed.\n",strlocalfilename,strlocalfilenamebak);
                return false;
            }
        }

    }

    return true;
}


bool LoadLocalFile() {

    CDir   Dir;         // 目录操作类

    struct st_fileinfo stfileinfo;

    vlistfile2.clear();

    // 设置时间的格式
    Dir.SetDateFMT("yyyymmddhh24miss");

    // 打开目录
    if (!Dir.OpenDir(starg.localpath,starg.matchname)) {
        logfile.Write("Dir.OpenDir(%s) failed.\n",starg.localpath);
        return false;
    }

    // 把客户端目录下的文件名加载到vlistfile2容器中
    while (true) {
        memset(&stfileinfo,0,sizeof(struct st_fileinfo));

        // 读取文件信息
        if (!Dir.ReadDir()) break;

        // 把文件信息加载到stfileinfo结构体中
        strcpy(stfileinfo.filename,Dir.m_FileName);
        strcpy(stfileinfo.mtime,Dir.m_ModifyTime);

        // 把stfileinfo结构体加入到vlistfile2容器中
        vlistfile2.push_back(stfileinfo);
    }    

    return true;
}


bool LoadOKFile() {

    CFile  File;  // 文件操作类

    struct st_fileinfo stfileinfo;

    vlistfile1.clear();

    // 打开文件
    if (!File.Open(starg.okfilename,"r")) return true;

    char strbuffer[301];
    while (true) {
        memset(&stfileinfo,0,sizeof(struct st_fileinfo));
        memset(strbuffer,0,sizeof(strbuffer));

        // 读取文件
        if (!File.Fgets(strbuffer,300,true)) break;

        // 解析xml,得到文件信息
        GetXMLBuffer(strbuffer,"filename",stfileinfo.filename,300);
        GetXMLBuffer(strbuffer,"mtime",stfileinfo.mtime,30);

        vlistfile1.push_back(stfileinfo);
    }
    return true;
} 


bool CompVector() {

    vlistfile3.clear(); vlistfile4.clear();

    int i,j;
    for (i = 0;i < vlistfile2.size();i++) {
        for (j = 0;j < vlistfile1.size();j++) {
            if (strcmp(vlistfile2[i].filename,vlistfile1[j].filename) == 0 && \
                strcmp(vlistfile2[i].mtime,vlistfile1[j].mtime) == 0) {
                    vlistfile3.push_back(vlistfile2[i]);
                    break;
                }
        }

        if (j == vlistfile1.size()) 
            vlistfile4.push_back(vlistfile2[i]);
    }

    return true;
}


bool WriteToOKFile() {

    CFile   File;   // 文件操作类

    // 打开文件
    if (!File.Open(starg.okfilename,"w")) {
        logfile.Write("File.Open(%s) failed.\n",starg.okfilename);
        return false;
    }

    // 写入文件
    for (int i = 0;i < vlistfile3.size();i++) 
        File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",vlistfile3[i].filename,vlistfile3[i].mtime);

    return true;
}


bool AppendToOKFile(struct st_fileinfo *stfileinfo) {

    CFile   File;   // 文件操作类

    // 打开文件
    if (!File.Open(starg.okfilename,"a")) {
        logfile.Write("File.Open(%s) failed.\n",starg.okfilename);
        return false;
    }

    // 写入文件
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);

    return true;
}

void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n",sig);
    exit(0);
}