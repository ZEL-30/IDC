#include "_connpool.h"



CConnPool::CConnPool() {

    m_conns = NULL;
    m_maxconns = 0;
    m_timeout = 0;
    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));

}


bool CConnPool::init(const char *connstr, const char *charset, int maxconns, int timeout) {

    // 尝试连接数据库, 验证数据库连接参数是否正确
    connection conn;
    if (conn.connecttodb(connstr, charset) != 0) {
        printf("连接数据库失败.\n%s\n", conn.m_cda.message);
        return false;
    }
    conn.disconnect();

    strncpy(m_connstr, connstr, 100); 
    strncpy(m_charset, charset, 100);
    m_maxconns = maxconns;
    m_timeout = timeout;

    // 分配数据库连接池的内存空间
    m_conns = new struct st_conn[m_maxconns];

    for (int i = 0; i < m_maxconns; i++) {
        // 初始化锁
        pthread_mutex_init(&m_conns[i].mutex, NULL);

        // 数据库连接上次使用的时间初始化为0
        m_conns[i].atime = 0;
    }

    return true;
}


connection* CConnPool::get() {

    int pos = -1;        // 用于记录第一个未连接数据库的数组位置

    for (int i = 0; i < m_maxconns; i++) {

        // 尝试加锁, 寻找空闲的数据库连接
        if (pthread_mutex_trylock(&m_conns[i].mutex) == 0) {

            // 如果数据已连接, 返回数据库连接的地址
            if (m_conns[i].atime > 0) {
                printf("get conn %d.\n", i);
                
                // 如果之前有加锁的未连接好的位置, 那么释放锁
                if (pos != -1) pthread_mutex_unlock(&m_conns[pos].mutex);

                // 把数据库连接的使用时间设置为当前时间
                m_conns[i].atime = time(NULL);

                // 返回数据库连接的地址
                return &m_conns[i].conn;
            }

            if (pos == -1) 
                pos = i;
            else
                pthread_mutex_unlock(&m_conns[i].mutex);     // 释放锁
        }
    }

    // 如果数据库连接已用完, 放回空
    if (pos == -1) {
        printf("数据库连接已用完.\n");
        return NULL;
    }

    // 连接池没有用完，让m_conns[pos].conn连上数据库。
    printf("get conn %d.\n",pos);

    // 连接数据库
    if (m_conns[pos].conn.connecttodb(m_connstr, m_charset) != 0) {
        printf("connect database failed.\n");

        pthread_mutex_unlock(&m_conns[pos].mutex);    // 释放锁
        return NULL;
    }

    // 把数据库连接的使用时间设置为当前时间
    m_conns[pos].atime = time(NULL);

    return &m_conns[pos].conn;
}


bool CConnPool::free(connection *conn) {

    for (int i = 0; i < m_maxconns; i++) {
        if (&m_conns[i].conn == conn) {
            printf("free conn %d.\n", i);
            pthread_mutex_unlock(&m_conns[i].mutex);    // 释放锁
            m_conns[i].atime = time(NULL);                       // 把数据库连接的使用时间设置为当前时间
            return true;
        }
    }

    return false;
}


void CConnPool::checkpool() {

    for (int i = 0; i < m_maxconns; i++) {
        if (pthread_mutex_trylock(&m_conns[i].mutex) == 0) {
            // 如果是一个可用的连接
            if (m_conns[i].atime > 0) {
                // 判断连接是否超时
                if (time(NULL) - m_conns[i].atime > m_timeout) {
                    m_conns[i].conn.disconnect();       // 断开数据库连接
                    m_conns[i].atime = 0;               // 重置数据库连接的使用时间
                }
            } else {
                // 如果没有超时, 执行一次SQL, 验证连接是否有效, 如果无效, 断开数据库连接
                if (m_conns[i].conn.execute("select * from dual") != 0) {
                    m_conns[i].conn.disconnect();       // 断开数据库连接
                    m_conns[i].atime = 0;               // 重置数据库连接的使用时间
                }
            }

            pthread_mutex_unlock(&m_conns[i].mutex);    // 释放锁
        }
    }

}


void CConnPool::destroy() {

    for (int i = 0; i < m_maxconns; i++) {
        // 断开数据库连接
        m_conns[i].conn.disconnect();

        // 销毁数据库连接的互斥锁
        pthread_mutex_destroy(&m_conns[i].mutex);
    }

    // 释放数据库连接池的内存空间
    delete[] m_conns;
    m_conns = NULL;

    memset(m_connstr, 0, sizeof(m_connstr));
    memset(m_charset, 0, sizeof(m_charset));
    m_timeout = 0;
    m_maxconns = 0;

}


CConnPool::~CConnPool() {
    destroy();
}