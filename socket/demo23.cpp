/*
 *  程序名: demo23.cpp, 本程序完成面试题
 *  文本文件中有m行, 每行只有一个数字, 写一个多线程程序, 启动n线程并行计算, 把计算结果汇总
 *  作者: ZEL
 */
#include "_public.h"


CLogFile   logfile;    // 日志文件操作类

int row = 0; 

vector<int> vnumber;

// 程序的帮助文档
void _help();

// 生成文件
bool crtFile(const char *filename);

// 加载文件
bool LoadFile(const char *filename);

// 并行计算线程主函数
void *thmain(void *arg);





int main(int argc, char const *argv[]) {

    if (argc != 3) {
        _help();
        return -1;
    }

    // 打开日志文件
    if (!logfile.Open(argv[2], "a")) {
        printf("logfile.Open(%s) failed.\n", argv[2]);
        return -1;
    }

    // 生成文件
    if (!crtFile(argv[1])) {
        logfile.Write("crtFile(%s) failed.\n", argv[1]);
        return -1;
    }

    // 加载文件
    if (!LoadFile(argv[1])) {
        logfile.Write("LoadFile() failed.\n");
        return -1;
    }
    printf("%d , %d\n", (1 + row/3), (1 + 2*(row/3)));

    CTimer Timer1;    // 计时

    // 创建线程
    pthread_t thid1 = 0, thid2 = 0, thid3 = 0;
    pthread_create(&thid1, NULL, thmain, (void *)(long)1);
    pthread_create(&thid2, NULL, thmain, (void *)(long)(1 + row/3));
    pthread_create(&thid3, NULL, thmain, (void *)(long)(1 + 2*(row/3)));

    // 等待线程退出
    void *ret1 = 0, *ret2 = 0, *ret3 = 0;
    pthread_join(thid1, &ret1);
    pthread_join(thid2, &ret2);
    pthread_join(thid3, &ret3);

    // 汇总计算结果
    int result = (int)(long)ret1 + (int)(long)ret2 + (int)(long)ret3;

    printf("并行的计算结果为 : %d 耗时 : %.02fsec\n", result, Timer1.Elapsed());


    CTimer Timer2;    // 计时
    // 校验
    int num = 0;
    for (int i = 0; i < 100; i++) {
        num += vnumber[i];
    }
    printf("校验的计算结果为 : %d 耗时 : %.02fsec\n", num, Timer2.Elapsed());


    return 0;
}



void _help() {
    printf("\n");
    printf("Using : ./demo23.out filename logfile\n");
    printf("Example : ./demo23.out demo23.txt /tmp/demo23.log\n");
}


bool crtFile(const char *filename) {

    CFile File;   // 文件操作类

    // 打开文件
    if (!File.Open(filename, "w+")) {
        logfile.Write("File.Open(%s) failed.\n", filename);
        return false;
    }

    // 写入文件
    for (int i = 0; i < 100; i++) {
        File.Fprintf("%d\n", i + 1);
    }

    // 关闭文件
    File.Close();

    return true;   
}


bool LoadFile(const char *filename) {

    CFile File;   // 文件操作类

    // 打开文件
    if (!File.Open(filename, "r")) {
        logfile.Write("File.Open(%s) failed.\n", filename);
        return false;
    }

    // 读取文件
    char strbuffer[1024];     
    while (true) {
        if (!File.Fgets(strbuffer, 1000, true)) break;

        vnumber.push_back(atoi(strbuffer));

        row++;
    }

    // 关闭文件
    File.Close();

    return true;
}


void *thmain(void *arg) {

    int start = (int)(long)arg;
    int result = 0;

    for (int i = start; i < start + (row/3); i++) {
        if (i - 1 > vnumber.size()) break;
        printf("%d ", vnumber[i - 1]);
        result += vnumber[i - 1];
    }

    printf("\n");
    printf("线程%lu, 计算内容 : %d - %d, 计算结果为 : %d\n", pthread_self(), start, start + (row/3), result);

    return (void *)result;
}