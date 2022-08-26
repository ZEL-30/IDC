/*
 *  程序名: ftpgetfiles.cpp, 本程序基于FTP实现文件下载功能
 *  作者: ZEL
 */

#include "_public.h"
#include "_ftp.h"


CLogFile  logfile;  // 日志文件操作类
Cftp      ftp;      // FTP操作类 
CPActive  PActive;  // 进程心跳操作类


// 运行参数的结构体
struct st_arg {
    char host[31];            // 远程服务端的IP和端口
    int  mode;                // 传输模式，1-被动模式，2-主动模式，缺省采用被动模式
    char username[31];        // 远程服务端ftp的用户名
    char password[31];        // 远程服务端ftp的密码
    char remotepath[301];     // 远程服务端存放文件的目录
    char localpath[301];      // 本地文件存放的目录
    char matchname[101];      // 待下载文件匹配的规则
    char listfilename[301];   // 下载前列出服务端文件名的文件
    int  ptype;               // 下载后服务端文件的处理方式：1-什么也不做；2-删除；3-备份
    char remotepathbak[301];  // 下载后服务端文件的备份目录
    char okfilename[301];     // 已下载成功文件名清单
    bool checkmtime;          // 是否需要检查服务端文件的时间，true-需要，false-不需要，缺省为false
    int  timeout;             // 进程心跳的超时时间
    char pname[51];           // 进程名，建议用"ftpgetfiles_后缀"的方式
}starg;

// 文件信息的结构体
struct st_fileinfo {
    char   filename[301];      // 文件名
    char   mtime[21];          // 文件时间
};


vector<struct st_fileinfo> vlistfile1;  // 已下载成功文件名的容器, 从okfilename中加载
vector<struct st_fileinfo> vlistfile2;  // 下载前列出服务器文件名的容器, 从nlist文件中加载
vector<struct st_fileinfo> vlistfile3;  // 本次不需要下载的文件名的容器
vector<struct st_fileinfo> vlistfile4;  // 本次需要下载的文件名的容器
 

// 程序帮助文档
void _help();

// 把xml解析到参数starg结构体中
bool _xmltoarg(const char *strxmlbuffer);

// 下载文件功能主函数
bool _ftpgetfiles();

// 把ftp.nlist()方法获取到的list文件加载到容器vlistfile2中。
bool LoadListFile();

// 加载okfilename文件中的内容到容器vlistfile1中
bool LoadOKFile();

// 比较vlistfile1和vlistfile2, 得到vlistfile3和vlistfile4
bool CompVector();

// 把容器vlistfile3中的内容写入okfilename文件, 覆盖之前的okfilename文件
bool WriteToOKFile();

// 把下载成功的文件记录追加到okfilename文件中
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

    // 解析xml, 得到程序运行的参数
    if (!_xmltoarg(argv[2])) {
        logfile.Write("_xmltoarg(%s) failed.\n",argv[2]);
        EXIT(-1);
    }

    // 把进程的心跳信息写入共享内存
    PActive.AddPInfo(starg.timeout,starg.pname);


    // 登录FTP服务器
    if (!ftp.login(starg.host,starg.username,starg.password,starg.mode)) {
        logfile.Write("ftp.login(%s,%s,%s) failed.\n",starg.host,starg.username,starg.password);
        EXIT(-1);
    }

    // 下载文件功能的主函数
    _ftpgetfiles();

    // 注销登录
    ftp.logout();

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/ftpgetfiles logfilename xmlbuffer\n\n");

    printf("Sample:/project/tools/bin/procctl 30 /project/tools/bin/ftpgetfiles /log/idc/ftpgetfiles_surfdata.log \"<host>127.0.0.1:21</host><mode>1</mode><username>zel</username><password>19981110</password><localpath>/idcdata/surfdata</localpath><remotepath>/tmp/idc/surfdata</remotepath><matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname><listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename><ptype>1</ptype><remotepathbak>/tmp/idc/surfdatabak</remotepathbak><okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename><checkmtime>true</checkmtime><timeout>80</timeout><pname>ftpgetfiles_surfdata</pname>\"\n\n\n");

    printf("本程序是通用的功能模块，用于把远程ftp服务端的文件下载到本地目录。\n");
    printf("logfilename是本程序运行的日志文件。\n");
    printf("xmlbuffer为文件下载的参数，如下：\n");
    printf("<host>127.0.0.1:21</host> 远程服务端的IP和端口。\n");
    printf("<mode>1</mode> 传输模式，1-被动模式，2-主动模式，缺省采用被动模式。\n");
    printf("<username>zel</username> 远程服务端ftp的用户名。\n");
    printf("<password>19981110</password> 远程服务端ftp的密码。\n");
    printf("<remotepath>/tmp/idc/surfdata</remotepath> 远程服务端存放文件的目录。\n");
    printf("<localpath>/idcdata/surfdata</localpath> 本地文件存放的目录。\n");
    printf("<matchname>SURF_ZH*.XML,SURF_ZH*.CSV</matchname> 待下载文件匹配的规则。"\
            "不匹配的文件不会被下载，本字段尽可能设置精确，不建议用*匹配全部的文件。\n");
    printf("<listfilename>/idcdata/ftplist/ftpgetfiles_surfdata.list</listfilename> 下载前列出服务端文件名的文件。\n");
    printf("<ptype>1</ptype> 文件下载成功后，远程服务端文件的处理方式：1-什么也不做；2-删除；3-备份，如果为3，还要指定备份的目录。\n");
    printf("<remotepathbak>/tmp/idc/surfdatabak</remotepathbak> 文件下载成功后，服务端文件的备份目录，此参数只有当ptype=3时才有效。\n");
    printf("<okfilename>/idcdata/ftplist/ftpgetfiles_surfdata.xml</okfilename> 已下载成功文件名清单，此参数只有当ptype=1时才有效。\n");
    printf("<checkmtime>true</checkmtime> 是否需要检查服务端文件的时间，true-需要，false-不需要，此参数只有当ptype=1时才有效，缺省为false。\n");
    printf("<timeout>80</timeout> 下载文件超时时间，单位：秒，视文件大小和网络带宽而定。\n");
    printf("<pname>ftpgetfiles_surfdata</pname> 进程名，尽可能采用易懂的、与其它进程不同的名称，方便故障排查。\n\n\n");
}


void EXIT(int sig) {
    logfile.Write("程序退出, sig = %d\n",sig);
    exit(0);
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

    GetXMLBuffer(strxmlbuffer,"listfilename",starg.listfilename,300);
    if (strlen(starg.listfilename) == 0) {
        logfile.Write("listfilename is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"ptype",&starg.ptype);
    if (starg.ptype != 1 && starg.ptype != 2 && starg.ptype != 3) {
        logfile.Write("ptype is error.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"remotepathbak",starg.remotepathbak,300);
    if (starg.ptype == 3 && strlen(starg.remotepathbak) == 0) {
        logfile.Write("remotepathbak is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"okfilename",starg.okfilename,300);
    if (starg.ptype == 1 && strlen(starg.okfilename) == 0) {
        logfile.Write("okfilename is null.\n");
        return false;
    }

    GetXMLBuffer(strxmlbuffer,"checkmtime",&starg.checkmtime);
    
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


bool _ftpgetfiles() {

    // 进入FTP服务器存放文件的目录
    if (!ftp.chdir(starg.remotepath)) {
        logfile.Write("Dir:%s isn't exist.\n",starg.remotepath);
        return false;
    }

    // 调用ftp.nlist()方法列出服务器目录中的文件,结果存放到本地文件中
    if (!ftp.nlist("*",starg.listfilename)) {
        logfile.Write("ftp.nlist(%s,%s) failed.\n",starg.remotepath,starg.listfilename);
        return false;
    }

    // 更新程序的心跳
    PActive.UptATime();

    // 把ftp.nlist()方法获取到的list文件加载到容器vlistfile2中
    if (!LoadListFile()) {
        logfile.Write("LoadListFile() failed.\n");
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

    char strremotefilename[301];      // 远程文件名
    char strlocalfilename[301];       // 本地文件名
    char strremotefilenamebak[301];   // 远程备份文件名

    // 遍历容器vlistfile2
    for (int i = 0;i < vlistfile2.size();i++) {

        memset(strremotefilename,0,sizeof(strremotefilename));
        memset(strlocalfilename,0,sizeof(strlocalfilename));
        
        sprintf(strremotefilename,"%s/%s",starg.remotepath,vlistfile2[i].filename);
        sprintf(strlocalfilename,"%s/%s",starg.localpath,vlistfile2[i].filename);

        // 开始下载文件
        logfile.Write("get %s ...",strremotefilename);

        // 调用ftp.get()方法从服务器下载文件
        if (!ftp.get(strremotefilename,strlocalfilename,true)) {
            logfile.WriteEx(" failed.\n");
            return false;
        }

        logfile.WriteEx(" ok.\n");

        // 更新程序的心跳
        PActive.UptATime();

        // 如果ptype=1,把下载成功的文件记录追加到okfilename文件中
        if (starg.ptype == 1) 
            AppendToOKFile(&vlistfile2[i]);
        

        // 如果ptype=2,删除服务器上的文件
        if (starg.ptype == 2) {
            if (!ftp.ftpdelete(strremotefilename)) {
                logfile.Write("ftp.ftpdeleta(%s) failed.\n",strremotefilename);
                return false;
            }
        }

        // 如果ptype=3,备份服务器上的文件
        if (starg.ptype == 3) {
            memset(strremotefilenamebak,0,sizeof(strremotefilenamebak));
            sprintf(strremotefilenamebak,"%s/%s",starg.remotepathbak,vlistfile2[i].filename);

            if (!ftp.ftprename(strremotefilename,strremotefilenamebak)) {
                logfile.Write("ftp.ftprename(%s,%s) failed.\n",strremotefilename,strremotefilenamebak);
                return false;
            }  
        }

    }

    return true;
}


bool LoadListFile() {

    CFile  File;            // 文件操作类

    struct st_fileinfo stfileinfo;

    // 清空vlistfile2容器
    vlistfile2.clear();

    // 打开文件
    if (!File.Open(starg.listfilename,"r")) {
        logfile.Write("%s isn't exist.\n");
        return true;
    }

    // 把list文件加载到vlistfile2中
    while (true) {
        memset(&stfileinfo,0,sizeof(struct st_fileinfo));

        // 读取文件
        if (!File.Fgets(stfileinfo.filename,300,true)) break;

        if (!MatchStr(stfileinfo.filename,starg.matchname)) continue;

        if (starg.ptype == 1 && starg.checkmtime == true) {
            // 获取FTP服务端文件时间
            if (!ftp.mtime(stfileinfo.filename)) {
                logfile.Write("ftp.mtime(%s) failed.\n",stfileinfo.filename);
                return false;
            }
            strcpy(stfileinfo.mtime,ftp.m_mtime);
        }

        // 把stfileinfo结构体加入到vlistfile2容器中
        vlistfile2.push_back(stfileinfo);
    }

    return true;
}


bool LoadOKFile() {

    CFile  File;     // 文件操作类

    struct st_fileinfo stfileinfo;

    char strbuffer[501];

    vlistfile1.clear();

    // 打开文件
    if (!File.Open(starg.okfilename,"r")) return true;

    // 读取文件
    while (true) {
        memset(strbuffer,0,sizeof(strbuffer));
        memset(&stfileinfo,0,sizeof(struct st_fileinfo));

        if (!File.Fgets(strbuffer,300,true)) break;

        GetXMLBuffer(strbuffer,"filename",stfileinfo.filename,300);
        GetXMLBuffer(strbuffer,"mtime",stfileinfo.mtime,20);

        vlistfile1.push_back(stfileinfo);
    }

    return true;
}


bool CompVector() {

    vlistfile3.clear();  vlistfile4.clear();

    int i,j;
    for (i = 0;i < vlistfile2.size();i++) {
        for (j = 0;j < vlistfile1.size();j++) {
            if ((strcmp(vlistfile2[i].filename,vlistfile1[j].filename) == 0) &&\
                (strcmp(vlistfile2[i].mtime,vlistfile1[j].mtime) == 0)) {
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

    CFile  File;       // 文件操作类
    
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


bool AppendToOKFile(struct st_fileinfo *stfileinfo){

    CFile  File;       // 文件操作类
    
    // 打开文件
    if (!File.Open(starg.okfilename,"a")) {
        logfile.Write("File.Open(%s) failed.\n",starg.okfilename);
        return false;
    }

    // 写入文件
    File.Fprintf("<filename>%s</filename><mtime>%s</mtime>\n",stfileinfo->filename,stfileinfo->mtime);
    
    return true;


}