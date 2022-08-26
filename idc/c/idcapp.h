/****************************************************************************************/
/*   程序名：idcapp.h，此程序是数据中心项目公用函数和类的声明文件                    */
/*   作者：ZEL                                                                       */
/****************************************************************************************/


#pragma once

#include "_public.h"
#include "_mysql.h"

// 全国气象站点分钟观测数据结构体
struct st_obtmind {
    char  obtid[11];       // 站点代码
    char  ddatetime[21];   // 数据时间, 精确到分钟
    char  t[11];           // 温度, 单位: 0.1摄氏度
    char  p[11];           // 气压, 单位: 0.1百帕
    char  u[11];           // 相对湿度, 0-100之间的值
    char  wd[11];          // 风向, 单位: 0-360之间的值
    char  wf[11];          // 风速: 单位: 0.1m/s
    char  r[11];           // 降雨量: 0.1mn
    char  vis[11];         // 能见度: 0.1米
};


// 全国站点分钟观测数据操作类
class CZHOBTMIND {

public:
    connection    *m_conn;          // 数据库连接
    CLogFile      *m_logfile;       // 日志文件操作
    sqlstatement   m_stmt;          // 插入表操作的SQL
    
    struct st_obtmind m_stobtmind;  // 全国站点观测数据的结构体

    char m_buffer[1024];

    CZHOBTMIND();

    CZHOBTMIND(connection *conn, CLogFile *logfile);

    // 把connection和CLogFile的指针传进去
    void BindConnLog(connection *conn, CLogFile *logfile);

    // 把从文件读到的一行数据拆分到m_stobtmind结构体中
    bool SplitBuffer(char *strBuffer, bool bisxml);

    // 把m_stobtmind结构体中的数据插入到T_ZHOBTMIND表中
    bool InsertTable();

    ~CZHOBTMIND();

private:

};