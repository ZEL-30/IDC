/*
 *  程序名: crtsurfdata.cpp, 本程序用于生成全国气象站点观测的分钟数据
 *  作者: ZEL
 */


#include "_public.h"


CLogFile logfile;  // 日志文件操作类
CFile    File;     // 文件操作类
CPActive PActive;  // 进程心跳操作类

// 全国气象站点参数结构体
struct st_stcode {
  char provname[31];  // 省
  char obtid[11];     // 站号
  char obtname[31];   // 站名
  double lat;         // 纬度
  double lon;         // 经度
  double height;      // 海拔高度
};

// 全国气象站点分钟观测数据结构体
struct st_surfdata {
  char obtid[11];      // 站点代码
  char ddatetime[21];  // 数据时间：格式yyyymmddhh24miss
  int t;               // 气温：单位，0.1摄氏度
  int p;               // 气压：0.1百帕
  int u;               // 相对湿度，0-100之间的值
  int wd;              // 风向，0-360之间的值
  int wf;              // 风速：单位0.1m/s
  int r;               // 降雨量：0.1mm
  int vis;             // 能见度：0.1米
};


vector<struct st_stcode> vstcode;      // 存放全国气象站点参数的容器
vector<struct st_surfdata> vsurfdata;  // 存放全国气象站点分钟观测数据的容器


char strddatetime[21];  // 观测数据的时间


// 程序帮助文档
void _help();

// 把站点参数文件的参数加载到容器中
bool LoadSTcode(const char *inifile);

// 模拟生成全国气象站点分钟观测数据,存放在vsurfdata容器中
void CrtSurfData();

// 把容器vsurfdata中的全国气象站点分钟观测数据写入文件
bool CrtSurfFile(const char *outpath, const char *datafmt);

// 程序退出和信号2,15的处理函数
void EXIT(int sig);




int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 5 && argc != 6) {
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
    if (!logfile.Open(argv[3], "a+")) {
        printf("logfile.Open(%s) failed.\n", argv[3]);
        return -1;
    }

    logfile.Write("crtsurfdata 开始运行.\n");

    // 设置心跳信息
    PActive.AddPInfo(20,"crtsurfdata");

    // 把站点参数文件的参数加载到容器中
    if (!LoadSTcode(argv[1])) {
        logfile.Write("LoadSTcode(%s) failed.\n", argv[1]);
        return -1;
    }

    // 获取当前时间,作为观测时间
    memset(strddatetime, 0, sizeof(strddatetime));
    if (argc == 5)
        LocalTime(strddatetime, "yyyymmddhh24miss");
    else
        STRCPY(strddatetime, sizeof(strddatetime),argv[5]);
    
    
    // 模拟生成全国气象站点分钟观测数据,存放在vsurfdata容器中
    CrtSurfData();

    // 把容器vsurfdata中的全国气象站点分钟观测数据写入文件
    if (strstr(argv[4], "xml") != NULL) CrtSurfFile(argv[2], "xml");
    if (strstr(argv[4], "csv") != NULL) CrtSurfFile(argv[2], "csv");
    if (strstr(argv[4], "json") != NULL) CrtSurfFile(argv[2], "json");

    logfile.Write("crtsurfdata 运行结束.\n");

    return 0;
}







void _help() {
    printf("\n");
    printf("Using:./crtsurfdata.out inifile outpath logfile datafmt [datetime]\n");
    printf("Example:/project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv\n");
    printf("        /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv 20210710123000\n");
    printf("        /project/tools/bin/procctl 60 /project/idc/bin/crtsurfdata /project/idc/ini/stcode.ini /tmp/idc/surfdata /log/idc/crtsurfdata.log xml,json,csv\n\n\n");

    printf("inifile  全国气象站点参数文件名。\n");
    printf("outpath  全国气象站点数据文件存放的目录。\n");
    printf("logfile  本程序运行的日志文件名。\n");
    printf("datafmt  生成数据文件的格式，支持xml、json和csv三种格式，中间用逗号分隔。\n");
    printf("datetime 这是一个可选参数，表示生成指定时间的数据和文件。\n\n\n");
}


bool LoadSTcode(const char *inifile) {
    CCmdStr CmdStr;      // 字符串拆分类
    
    // 打开站点参数文件
    if (!File.Open(inifile, "r")) {
        logfile.Write("File.Open(%s) failed.\n", inifile);
        return false;
    }

    struct st_stcode stcode;
    char strBuffer[301];

    // 从站点参数文件读取一行,如果读取完成,跳出循环
    while (true) {
        memset(&stcode, 0, sizeof(struct st_stcode));

        if (!File.Fgets(strBuffer, 300, true)) break;

        // 拆分得到的字符串
        CmdStr.SplitToCmd(strBuffer, ",", true);

        // 扔掉无效行
        if (CmdStr.CmdCount() != 6) continue;

        // 把站点参数的每一行数据保存到站点参数结构体中
        CmdStr.GetValue(0, stcode.provname, 30);
        CmdStr.GetValue(1, stcode.obtid, 10);
        CmdStr.GetValue(2, stcode.obtname, 30);
        CmdStr.GetValue(3, &stcode.lat);
        CmdStr.GetValue(4, &stcode.lon);
        CmdStr.GetValue(5, &stcode.height);

        // 把站点参数结构体放入站点参数容器
        vstcode.push_back(stcode);
    }

    return true;
}


void CrtSurfData() {
    // 播下随机数种子
    srand((unsigned int)time(NULL));

    struct st_surfdata stsurfdata;

    // 遍历气象站点参数的vscode容器
    for (int i = 0; i < vstcode.size(); i++) {
        memset(&stsurfdata, 0, sizeof(struct st_surfdata));

        // 用随机数充填分钟观测数据的结构体
        strncpy(stsurfdata.obtid, vstcode[i].obtid, 10);     // 站点代码
        strncpy(stsurfdata.ddatetime, strddatetime, 14);     // 数据时间：格式yyyymmddhh24miss
        stsurfdata.t = rand() % 351;                         // 气温：单位，0.1摄氏度
        stsurfdata.p = rand() % 265 + 10000;                 // 气压：0.1百帕
        stsurfdata.u = rand() % 100 + 1;                     // 相对湿度，0-100之间的值。
        stsurfdata.wd = rand() % 360;                        // 风向，0-360之间的值。
        stsurfdata.wf = rand() % 150;                        // 风速：单位0.1m/s
        stsurfdata.r = rand() % 16;                          // 降雨量：0.1mm
        stsurfdata.vis = rand() % 5001 + 100000;             // 能见度：0.1米

        // 把观测数据的结构体放入vsurfdata容器
        vsurfdata.push_back(stsurfdata);
    }
}

bool CrtSurfFile(const char *outpath, const char *datafmt) {
    
    // 拼接生成数据的文件名,例如：/tmp/idc/surfdata/SURF_ZH_20210629092200_2254.csv
    char strFileName[301];
    memset(strFileName, 0, sizeof(strFileName));
    sprintf(strFileName, "%s/SURF_ZH_%s_%d.%s", outpath, strddatetime, getpid(),
            datafmt);

    // 打开文件,创建临时文件
    if (!File.OpenForRename(strFileName, "w")) {
        logfile.Write("File.OpenForRename(%s) failed.\n", strFileName);
        return false;
    }

    // 写入第一行标题
    if (strcmp(datafmt, "csv") == 0)
        File.Fprintf(
            "站点代码,数据时间,气温,气压,相对湿度,风向,风速,降雨量,能见度\n");
    if (strcmp(datafmt, "xml") == 0) File.Fprintf("<data>\n");
    if (strcmp(datafmt, "json") == 0) File.Fprintf("{\"data\":[\n");

    // 遍历存放观测数据的vsurfdata容器
    for (int i = 0; i < vsurfdata.size(); i++) {
        // 为csv文件写入记录
        if (strcmp(datafmt, "csv") == 0) {
        File.Fprintf("%s,%s,%.1f,%.1f,%d,%d,%.1f,%.1f,%.1f\n", vsurfdata[i].obtid,
                    vsurfdata[i].ddatetime, vsurfdata[i].t / 10.0,
                    vsurfdata[i].p / 10.0, vsurfdata[i].u, vsurfdata[i].wd,
                    vsurfdata[i].wf / 10.0, vsurfdata[i].r / 10.0,
                    vsurfdata[i].vis / 10.0);
        }

        // 为xml文件写入记录
        if (strcmp(datafmt, "xml") == 0) {
        File.Fprintf(
            "<obtid>%s</obtid><ddatetime>%s</ddatetime><t>%.1f</t><p>%.1f</"
            "p><u>%d</u><wd>%d</wd><wf>%.1f</wf><r>%.1f</r><vis>%.1f</vis><endl/"
            ">\n",
            vsurfdata[i].obtid, vsurfdata[i].ddatetime, vsurfdata[i].t / 10.0,
            vsurfdata[i].p / 10.0, vsurfdata[i].u, vsurfdata[i].wd,
            vsurfdata[i].wf / 10.0, vsurfdata[i].r / 10.0,
            vsurfdata[i].vis / 10.0);
        }

        // 为json文件写入记录
        if (strcmp(datafmt, "json") == 0) {
        File.Fprintf(
            "{\"obtid\":\"%s\",\"ddatetime\":\"%s\",\"t\":\"%.1f\",\"p\":\"%."
            "1f\",\"u\":\"%d\",\"wd\":\"%d\",\"wf\":\"%.1f\",\"r\":\"%.1f\","
            "\"vis\":\"%.1f\"}",
            vsurfdata[i].obtid, vsurfdata[i].ddatetime, vsurfdata[i].t / 10.0,
            vsurfdata[i].p / 10.0, vsurfdata[i].u, vsurfdata[i].wd,
            vsurfdata[i].wf / 10.0, vsurfdata[i].r / 10.0,
            vsurfdata[i].vis / 10.0);

        // 处理最后一行记录的逗号
        if (i < vsurfdata.size() - 1)
            File.Fprintf(",\n");
        else
            File.Fprintf("\n");
        }
    }

    // 为xml文件写入结束标志
    if (strcmp(datafmt, "xml") == 0) File.Fprintf("</data>\n");

    // 为json文件写入结束标志
    if (strcmp(datafmt, "json") == 0) File.Fprintf("]}\n");

    // 关闭临时文件,并把临时文件改名为正式文件
    File.CloseAndRename();

    // 修改文件的时间属性
    UTime(strFileName,strddatetime);

    logfile.Write("生成数据文%s成功,数据时间%s,记录数%d.\n", strFileName,
                    strddatetime, vsurfdata.size());

    return true;
}


void EXIT(int sig) {
    logfile.Write("程序退出,sig = %d.\n\n",sig);

    exit(0);
}