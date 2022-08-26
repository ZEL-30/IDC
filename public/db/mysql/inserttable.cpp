/*
 *  程序名：inserttable.cpp，此程序演示开发框架操作MySQL数据库（向表中插入5条记录）
 *  作者：吴从周。
*/
#include <unistd.h>
#include "_mysql.h"       // 开发框架操作MySQL的头文件
#include "_public.h"

connection  conn;

struct st_girls {
    int     id;          // 编号
    char    name[31];    // 姓名
    double  weight;      // 体重
    char    btime[31];   // 报名时间
} stgirls;




int main(int argc, char const *argv[]) {
    
    CCmdStr   CmdStr;
    

    // 登录数据库
    if (conn.connecttodb("127.0.0.1,root,19981110,test,3306", "utf8") != 0) {
        printf("connect database failed.\n%s\n", conn.m_cda.message);
        return -1;
    }
    printf("connect database ok.\n");



    sqlstatement stmt(&conn);     // SQL语句操作类
    stmt.prepare("\
                insert into girls(id, name, weight, btime) \
                values(:1, :2, :3, str_to_date(:4, '%%Y-%%m-%%d %%H:%%i:%%s'))");
    
    // stmt.prepare("\
    // insert into girls(id,name,weight,btime) values(:1+1,:2,:3+45.35,str_to_date(:4,'%%Y-%%m-%%d %%H:%%i:%%s'))");
    stmt.bindin(1, &stgirls.id);
    stmt.bindin(2, stgirls.name, 30);
    stmt.bindin(3, &stgirls.weight);
    stmt.bindin(4, stgirls.btime, 30);
    
    string names = "貂蝉, 西施, 杨玉环, 王昭君, 江疏影";
    CmdStr.SplitToCmd(names, ",");

    for (int i = 0; i < 5; i++) {

        // 变量赋值
        stgirls.id = i + 1;
        strcpy(stgirls.name, CmdStr.m_vCmdStr[i].c_str());
        stgirls.weight = 45.5 + i;
        sprintf(stgirls.btime,"2022-08-01 10:33:%02d",i);


        // 执行SQL
        if (stmt.execute() != 0) {    
            printf("stmt.execute() failed.\n%s\n%s\n",stmt.m_sql,stmt.m_cda.message); 
            return -1;
        }

        // stmt.m_cda.rpc是本次执行SQL影响的记录数
        printf("成功插入了%ld条记录。\n",stmt.m_cda.rpc);
    }


    // 提交事务
    conn.commit();

    printf("insert table girls ok.\n");

    return 0;
}

