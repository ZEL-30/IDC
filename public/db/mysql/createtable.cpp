/*
 *  程序名: createtable.cpp, 此程序演示开发框架操作MySQL数据库（创建表）
 *  作者: ZEL
 */
#include "_mysql.h"

connection conn;     // MySQL数据库连接类


int main(int argc, char *argv[]) {

    // 登录数据库
    if (conn.connecttodb("127.0.0.1,root,19981110,test,3306", "utf8") != 0) {
        printf("connect database failed.\n%s\n", conn.m_cda.message);
        return -1;
    }
    printf("connect database ok.\n");

    sqlstatement stmt(&conn);     // SQL语句操作类
    
    // 准备SQL语句
    stmt.prepare("create table girls(id      bigint(10), \
                   name    varchar(30),   \
                   weight  decimal(8,2),  \
                   btime   datetime,      \
                   memo    longtext,      \
                   pic     longblob,      \
                   primary key (id))");

    // 执行SQL语句
    if (stmt.execute() != 0) {
        printf("stmt.execute() failed.\n%s\n%s\n", stmt.m_cda.rc, stmt.m_cda.message);
        return -1;
    }

    printf("create table girls ok.\n");

    return 0;
}




/*
-- 超女基本信息表。
create table girls(id      bigint(10),    -- 超女编号。
                   name    varchar(30),   -- 超女姓名。
                   weight  decimal(8,2),  -- 超女体重。
                   btime   datetime,      -- 报名时间。
                   memo    longtext,      -- 备注。
                   pic     longblob,      -- 照片。
                   primary key (id));
*/


