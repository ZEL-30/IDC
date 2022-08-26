#ifndef  _CONNPOOL_H
#define  _CONNPOOL_H

#include "_public.h"
#include "_ooci.h"


// 数据库连接池类
class CConnPool {

public:

    // 构造函数
    CConnPool();

    // 初始化数据库连接池, 初始化锁, 如果数据库连接参数有问题, 返回false
    bool init(const char *connstr, const char *charset, int maxconns, int timeout);

    // 1. 从数据库连接池中寻找一个空闲的, 已连接好的connection, 如果找到了, 放回它的地址
    // 2. 如果没有找到, 在连接池中找一个未连接的数据库connection, 连接数据库, 如果成功, 返回connection的地址
    // 3. 如果第二步找到了未连接数据库的conection, 但连接数据库失败, 返回空
    // 4. 如果第二步没有找到未连接数据库的conection, 表示数据库连接已用完, 也返回空
    connection *get();

    // 归还数据库连接
    bool free(connection *conn);

    // 检查数据库连接池, 断开空闲的连接, 在服务程序中, 用一个专用的子进程调用此函数
    void checkpool();

    // 断开数据库连接, 销毁锁, 释放数据库连接池的内存空间
    void destroy();

    // 析构函数
    ~CConnPool();

private:

    struct st_conn {
        connection       conn;        // 数据库连接
        pthread_mutex_t  mutex;       // 用于数据库连接的互斥锁  
        time_t           atime;       // 数据库连接上次使用的时间, 如果未连接取值为0
    } *m_conns;

    int      m_maxconns;              // 数据库连接池的最大值
    time_t   m_timeout;               // 数据库连接的超时时间, 单位: 秒
    char     m_connstr[101];          // 数据库连接参数: 用户/密码@连接名
    char     m_charset[101];          // 数据库的字符集

};


#endif