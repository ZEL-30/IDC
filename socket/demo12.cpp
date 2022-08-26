/*
 *  程序名: demo12.cpp, 本程序用于演示网银APP软件的服务端
 *  作者: ZEL
 */
#include "_public.h"


CTcpServer  TcpServer;  // socket通讯的服务端类
CLogFile    logfile;    // 日志文件操作类

char strsendbuffer[102400];
char strrecvbuffer[102400];

bool bsession = false;  // 客户端是否已登录, true-已登录; false-未登录或登录失败


// 程序帮助文档
void _help();

// 业务处理的主函数
bool _main(const char *strercvbuffer, char *strsendbuffer);

// 登录业务
bool srv001(const char *strrecvbuffer, char *strsendbuffer);

// 查询余额
bool srv002(const char *strrecvbuffer, char *strsendbuffer);

// 转账
bool srv003(const char *strrecvbuffer, char *strsendbuffer);

// 父进程退出和信号2、15的处理函数
void FathEXIT(int sig);

// 子进程退出和信号2、15的处理函数
void ChldEXIT(int sig);






int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    // 打开日志文件
    if (!logfile.Open(argv[2], "a+")) {
        printf("logfile.Open(%s) failed.\n",argv[2]);
        return -1;
    }

    // 关闭全部的信号和输入输出。
    // 设置信号,在shell状态下可用 "kill + 进程号" 正常终止些进程
    // 但请不要用 "kill -9 +进程号" 强行终止
    CloseIOAndSignal();
    signal(SIGINT, FathEXIT); signal(SIGTERM, FathEXIT);

    // 服务端初始化
    if (!TcpServer.InitServer(atoi(argv[1]))) {
        logfile.Write("TcpClient.InitServerr(%s) failed.\n",argv[1]);
        return -1;
    }


    while (true) {
        // 等待客户端的连接请求
        if (!TcpServer.Accept()) {
            logfile.Write("TcpClient.Accept() failed.\n");
            FathEXIT(-1);
        }
        
        logfile.Write("客户端(%s)已连接\n",TcpServer.GetIP());

        // 创建子进程, 如果是父进程, 关闭客户端的socket, 然后continue继续监听客户端请求
        if (fork() > 0) {
            // 关闭用于监听的socket
            TcpServer.CloseClient();
            continue;
        }

        // 子进程退出信号
        signal(SIGINT, ChldEXIT); signal(SIGTERM, ChldEXIT);

        // 子进程, 关闭监听的socket
        TcpServer.CloseListen();

        // 子进程与客户端通信，接收客户端发过来的报文后，回复ok
        while (true) {
            memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
            memset(strsendbuffer, 0, sizeof(strsendbuffer));
            
            // 接收客户端的请求报文
            if (!TcpServer.Read(strrecvbuffer)) break;
            logfile.Write("接收：%s\n", strrecvbuffer);

            // 处理业务的主函数
            if (!_main(strrecvbuffer,strsendbuffer)) break;

            // 向客户端发送回应报文
            logfile.Write("发送：%s\n", strsendbuffer);
            if (!TcpServer.Write(strsendbuffer, strlen(strsendbuffer))) break;
        }

        ChldEXIT(0);
    }

    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo12 port logfile\nExample:./demo12.out 5005 /tmp/demo12.log\n\n");
}


// 业务处理的主函数
bool _main(const char *strercvbuffer, char *strsendbuffer){

    // 解析客户端的请求报文, 获得服务代码(业务代码)
    int isrvcode;
    GetXMLBuffer(strrecvbuffer, "srvcode", &isrvcode);

    if ( (isrvcode!=1) && (bsession==false) ) {
        strcpy(strsendbuffer,"<retcode>-1</retcode><message>用户未登录。</message>"); 
        return true;
    }

    // 处理每种业务
    switch (isrvcode) {
        case 1:  // 登录业务
            srv001(strrecvbuffer, strsendbuffer); break;
        case 2:  // 查询余额
            srv002(strrecvbuffer, strsendbuffer); break;
        case 3:  // 转账
            srv003(strrecvbuffer, strsendbuffer); break;
        default:
            logfile.Write("业务代码不合法：%s\n",strrecvbuffer); return false;
    }

    return true;
}


bool srv001(const char *strrecvbuffer, char *strsendbuffer) {
    
    // 判断用户是否登录
    if (bsession == false) return false;

    // 解析得到电话和密码
    char tel[12], password[31];
    GetXMLBuffer(strrecvbuffer, "tel", tel, 11);
    GetXMLBuffer(strrecvbuffer, "password", password, 30);

    // 校验电话和密码
    if ((strcmp(tel ,"1502383271") != 0) && (strcmp(password, "19981110") != 0)) {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        strcpy(strsendbuffer, "<retcode>-1</retcode><message>失败</message>");
    } else {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        strcpy(strsendbuffer, "<retcode>0</retcode><message>成功</message>");
        bsession = true;
    }
    return true;
}


bool srv002(const char *strrecvbuffer, char *strsendbuffer) {

    // 判断用户是否登录
    if (bsession == false) return false;

    // 解析得到卡号
    char cardid[31];
    GetXMLBuffer(strrecvbuffer, "cardid", cardid, 30);

    // 校验卡号
    if (strcmp(cardid ,"626215125721927") != 0) {
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        strcpy(strsendbuffer, "<retcode>-1</retcode><message>失败</message>");
    } else {
        // 查询余额(通过数据库等), 得到余额
        double balance = 502475.56;
        memset(strsendbuffer, 0, sizeof(strsendbuffer));
        sprintf(strsendbuffer, "<retcode>0</retcode><message>成功</message><balance>%.2f</balance>",balance);
    }

    return true;
}


bool srv003(const char *strrecvbuffer, char *strsendbuffer) {

    // 判断用户是否登录
    if (bsession == false) return false;


    return true;
}


void FathEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    // 关闭监听的socket
    TcpServer.CloseListen();

    // 向所有的子进程发送退出信号
    kill(0,SIGTERM);

    // 父进程退出
    logfile.Write("父进程退出, sig = %d\n",sig);
    exit(0);
}

void ChldEXIT(int sig) {
    // 以下代码是为了防止信号处理函数在执行的过程中被信号中断
    signal(SIGINT, SIG_IGN); signal(SIGTERM, SIG_IGN);

    //关闭客户端的socket
    TcpServer.CloseClient();

    // 子进程退出
    logfile.Write("子进程退出, sig = %d\n",sig);
    exit(0);
}