/*
 *  程序名: deletefiles.cpp, 本程序实现文件清理功能
 *  作者: ZEL
 */

#include "_public.h"


CDir  Dir;  // 目录操作类


// 程序帮助文档
void _help();

// 程序退出和信号2,15的处理函数
void EXIT(int sig);





int main(int argc, char *argv[]) {
    
    // 程序帮助文档
    if (argc != 4) {
        _help();
        return -1;
    }
    
    // 关闭全部信号和输入输出
    // 设置信号，在shell状态下可用 "kill + 进程号" 正常终止进程
    // 但请不要用 "kill -9 + 进程号" 强行终止
    CloseIOAndSignal(true);
    signal(SIGINT,EXIT);
    signal(SIGTERM,EXIT);

    // 获取文件超时时间点
    char strTimeOut[21];
    memset(strTimeOut,0,sizeof(strTimeOut));
    LocalTime(strTimeOut,"yyyy-mm-dd hh24:mi:ss",0 - (int)(atof(argv[3])*24*60*60));

    // 打开目录
    if (!Dir.OpenDir(argv[1],argv[2],10000,true)) {
        printf("Dir.OpenDir(%s) failed.\n",argv[1]);
        return -1;
    }

    // 遍历目录中的文件
    while (true) {
        // 得到一个文件信息
        if (!Dir.ReadDir()) break;

        // 与超时的时间点比较，如果更早，就需要删除
        if (strcmp(strTimeOut,Dir.m_ModifyTime) > 0) {
            // 如果文件已超时,删除文件
            if (REMOVE(Dir.m_FullFileName))
                printf("REMOVE(%s) ok.\n",Dir.m_FullFileName);
            else
                printf("REMOVE(%s) failed.\n",Dir.m_FullFileName);   
        }

    }


    return 0;
}








void _help() {
    printf("\n");
    printf("Using:/project/tools/bin/deletefiles pathname matchstr timeout\n\n");

    printf("Example:/project/tools/bin/deletefiles /log/idc \"*.log.20*\" 0.02\n");
    printf("        /project/tools/bin/deletefiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n");
    printf("        /project/tools/bin/procctl 300 /project/tools/bin/deletefiles /log/idc \"*.log.20*\" 0.02\n");
    printf("        /project/tools/bin/procctl 300 /project/tools/bin/deletefiles /tmp/idc/surfdata \"*.xml,*.json\" 0.01\n\n");

    printf("这是一个工具程序，用于删除历史的数据文件或日志文件。\n");
    printf("本程序把pathname目录及子目录中timeout天之前的匹配matchstr文件全部删除，timeout可以是小数。\n");
    printf("本程序不写日志文件，也不会在控制台输出任何信息。\n\n\n");
}


void EXIT(int sig) {
    printf("程序退出,sig = %d.\n\n",sig);

    exit(0);
}