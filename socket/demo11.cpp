/*
 *  程序名: demo11.cpp, 本此程序用于演示网银APP软件的客户端
 *  作者: ZEL
 */
#include "_public.h"


CTcpClient TcpClient;  // socket通讯的客户端类

char strsendbuffer[1024];   // 向服务端发送的buffer
char strrecvbuffer[1024];   // 接收服务端发送的buffer


// 程序帮助文档
void _help();

// 处理业务的主函数
bool _main();

// 登录业务
bool srv001();

// 我的账户(查询余额)
bool srv002();

// 转账
bool srv003();







int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 3) {
        _help();
        return -1;
    }

    // 向服务端发起连接请求
    if (!TcpClient.ConnectToServer(argv[1], atoi(argv[2]))) {
      printf("TcpClient.ConnectToServer(%s,%s) failed.\n", argv[1], argv[2]);
      return -1;
    }

    // 处理业务的主函数
    _main();


    return 0;
}








void _help() {
    printf("\n");
    printf("Using:./demo11 ip port\nExample:./demo11.out 127.0.0.1 5005\n\n"); 
}


bool _main(){
    
    printf("**********************\n");
    printf("***欢迎使用ZEL网上银行***\n");
    printf("***    是否登录      ***\n");
    printf("***    1: 登录      ***\n");
    printf("***    2: 退出      ***\n");
    printf("**********************\n");
    int select = 0;
    scanf("%d", &select);

    bool status;
    switch (select) {
        case 1:  // 登录
            status = srv001(); break;
        case 2:  // 退出
            exit(0); break;
        default: return false;
    }

    // 登录业务
    if(status) {
        printf("**********************\n");
        printf("***欢迎使用ZEL网上银行***\n");
        printf("*** zel   登录成功   ***\n");
        printf("***    1: 查询余额   ***\n");
        printf("***    2: 转账      ***\n");
        printf("***    3: 退出      ***\n");
        printf("**********************\n");
        scanf("%d", &select);

        switch (select) {
            case 1:  // 查询余额
                srv002(); break;
            case 2:  // 转账
                srv003(); break;
            case 3:  // 退出
                exit(0); break;
            default: return false;
        }
    } else {
        printf("登录失败.\n");
        return false;
    }

    
    return true;
}

bool srv001() {

    char tel[12];
    char password[31];
    printf("请输入电话号码: \n");
    scanf("%s",tel);
    printf("请输入密码: \n");
    scanf("%s",password);

    // 向服务端发送请求报文
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<srvcode>1</srvcode><tel>%s</tel><password>%s</password>",tel,password);
    if (!TcpClient.Write(strsendbuffer)) return false;

    // 接收服务端的回应报文
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (!TcpClient.Read(strrecvbuffer)) return false;

    // 解析服务端返回的结果
    int iretcode = -1;
    GetXMLBuffer(strrecvbuffer, "retcode", &iretcode);
    if (iretcode != 0) {
        printf("登录失败.\n");
        return false;
    }

    printf("登录成功.\n");

    return true;
}


bool srv002() {

    char cardid[31];
    printf("请输入卡号: \n");
    scanf("%s",cardid);

    // 向服务端发送请求报文
    memset(strsendbuffer, 0, sizeof(strsendbuffer));
    SPRINTF(strsendbuffer, sizeof(strsendbuffer), "<srvcode>2</srvcode><cardid>%s</cardid>",cardid);
    if (!TcpClient.Write(strsendbuffer)) return false;

    // 接收服务端的回应报文
    memset(strrecvbuffer, 0, sizeof(strrecvbuffer));
    if (!TcpClient.Read(strrecvbuffer)) return false;

    // 解析服务端返回的结果
    int iretcode = -1;
    GetXMLBuffer(strsendbuffer, "retcode", &iretcode);
    if (iretcode != 0) {
        printf("查询余额失败.\n");
        return false;
    }

    double balance = 0;
    GetXMLBuffer(strrecvbuffer, "balance", &balance);

    printf("查询余额成功(%.2f).\n",balance);

    return true;
}


bool srv003(){



    return true;
}