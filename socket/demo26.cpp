/*
 *  程序名: demo26.cpp, 本程序用于演示HTTP协议的数据访问接口的简单实现
 *  作者: ZEL
 */
#include "_public.h"
#include "_ooci.h"


CTcpServer  TcpServer;  // socket通讯的服务端类

// 程序帮助文档
void _help();

// 解析GET请求中的参数，从T_ZHOBTMIND1表中查询数据，返回给客户端
bool SendData(const int sockfd,const char *strget);

// 从GET请求中获取参数的值：strget-GET请求报文的内容；name-参数名；value-参数值；len-参数值的长度
bool getvalue(const char *strget,const char *name,char *value,const int len);

// 生成HTML文件
bool crtHtml(const char *obtid);

// 发送HTML文件
bool sendFile(const int sockfd, const char *filename);


int main(int argc, char *argv[]) {

    // 程序帮助文档
    if (argc != 2) {
        _help();
        return -1;
    }

    // 服务端初始化
    if (!TcpServer.InitServer(atoi(argv[1]))) {
        printf("TcpClient.InitServerr(%s) failed.\n",argv[1]);
        return -1;
    }

    // 接收客户端的连接请求
    if (!TcpServer.Accept()) {
        printf("TcpClient.Accept() failed.\n");
        return -1;
    }
    printf("客户端(%s)已连接.\n", TcpServer.GetIP());    

    char strget[102400]; 
    char strsend[102400]; 

    // 接收HTTP客户端发送过来的报文
    memset(strget, 0, sizeof(strget));
    recv(TcpServer.m_connfd, strget, 1000, 0);
    printf("%s\n", strget);

    // 先把响应报文的头部发送给客户端
    memset(strsend, 0, sizeof(strsend));
    SPRINTF(strsend, sizeof(strsend), "HTTP/1.1 200 OK\r\n"
                                                  "Server: webserber\r\n"
                                                  "Content-Type: text/html;charset = utf-8\r\n"
                                                  "\r\n");
                                                //   "Content-Length: 105413\r\n\r\n");
    send(TcpServer.m_connfd, strsend, strlen(strsend), 0);


    // 解析GET请求中的参数, 从T_ZHOBTMIND1表中查询数据, 返回给客户端
    SendData(TcpServer.m_connfd, strget);


    return 0;
}





void _help() {
    printf("\n");
    printf("Using:./demo26 port\nExample:./demo26.out 5080\n\n");
}



bool SendData(const int sockfd, const char *strget) {

    // 解析URL中的参数。
    // 权限控制：用户名和密码。
    // 接口名：访问数据的种类。
    // 查询条件：设计接口的时候决定。 
    // http://127.0.0.1:8080/api?wucz&wuczpwd&getZHOBTMIND1&51076&20211024094318&20211024114020
    // http://124.70.51.197:5080/api?username=ZEL&passwd=19981110&intetname=getZHOBTMIND1&obtid=51076&begintime=20211024094318&endtime=20211024114020

    char username[31],passwd[31],intername[31],obtid[11],begintime[21],endtime[21];
    memset(username,0,sizeof(username));
    memset(passwd,0,sizeof(passwd));
    memset(intername,0,sizeof(intername));
    memset(obtid,0,sizeof(obtid));
    memset(begintime,0,sizeof(begintime));
    memset(endtime,0,sizeof(endtime));

    // 解析strget
    getvalue(strget, "username", username, 30);
    getvalue(strget, "passwd", passwd, 30);
    getvalue(strget, "intername", intername, 30);
    getvalue(strget, "obtid", obtid, 10);
    getvalue(strget, "begintime", begintime, 20);
    getvalue(strget, "endtime", endtime, 20);

    // 校验身份
    if (strcmp(username, "ZEL") != 0 || strcmp(passwd, "19981110") != 0) {
        // 向客户端发送错误信息
        char buffer[1024];
        memset(buffer, 0, sizeof(buffer));
        
        strcpy(buffer, "<!DOCTYPE html>  \
                        <html>  \
                            <head>  \
                                <meta charset=\"utf-8\">  \
                            </head>  \
                            <body>  \
                                <h1>错误!</h1>  \
                                <p>用户名或密码错误!</p>  \
                            </body>  \
                        </html>");
        send(sockfd, buffer, strlen(buffer), 0); 
        return false;
    }

    // 生成HTML文件
    if (!crtHtml(obtid)) {
        printf("crtHtml failed.\n");
        return false;
    }

    // 发送HTML文件
    sendFile(sockfd, "qxidc.html");

    return true;

}


bool getvalue(const char *strget, const char *name, char *value, const int len) {

    value[0] = 0;

    char *start = NULL, *end = NULL;
    
    start = strstr((char *)strget, (char *)name);
    if (start == NULL) return false;

    end = strstr(start, "&");
    if (end == NULL) end = strstr(start, " ");
    if (end == NULL) return false;

    int ilen = strlen(start) - strlen(end) - strlen(name) - 1;
    if (ilen > len) ilen = len;

    strncpy(value, start + strlen(name) + 1, ilen);

    value[ilen] = 0; 

    return true;
}



bool crtHtml(const char *obtid) {

    CFile File;    // 文件操作类

    char strfieldname[100][31];

    // 连接数据库
    connection conn;
    if (conn.connecttodb("qxidc/19981110@snorcl11g_197","Simplified Chinese_China.AL32UTF8") != 0) {
        printf("connect database failed.\n");
        return false;
    }
    printf("connect database(qxidc/19981110@snorcl11g_197) ok.\n");    

    sqlstatement stmt(&conn);
    stmt.prepare("select * from T_ZHOBTMIND1 where obtid = %s", obtid);
    for (int i = 1; i <= 11; i++){
        stmt.bindout(i, strfieldname[i - 1], 30);
    }
    
    // 执行SQL语句
    if (stmt.execute() != 0) {
        printf("stmt.execute() faile.\n%s\n%s\n", stmt.m_sql, stmt.m_cda.message);
        return false;
    }

    // 打开文件
    if (!File.Open("qxidc.html", "w")) {
        printf("File.Open() failed.\n");
        return false;
    }

    // 写入头部信息
    File.Fprintf("<!DOCTYPE html>    \
                <html>   \
                <meta charset=\"utf-8\">   \
                <title>数据中心</title>   \
                <head>   \ 
                    <style type=\"text/css\">   \
                        table.altrowstable {   \
                            font-family: verdana,arial,sans-serif;   \
                            font-size:11px;   \
                            color:#333333;   \
                            border-width: 1px;   \
                            border-color: #a9c6c9;   \
                            border-collapse: collapse;   \
                        }   \
                        table.altrowstable th {   \
                            border-width: 1px;   \
                            padding: 8px;   \
                            border-style: solid;   \
                            border-color: #a9c6c9;   \
                        }   \
                        table.altrowstable td {   \
                            border-width: 1px;   \
                            padding: 8px;   \
                            border-style: solid;   \
                            border-color: #a9c6c9;   \
                        }   \
                        .oddrowcolor{   \
                            background-color:#d4e3e5;   \
                        }   \
                        .evenrowcolor{   \
                            background-color:#c3dde0;   \
                        }   \
                    </style>   \
                    <script type=\"text/javascript\">   \
                        function altRows(id){   \
                            if(document.getElementsByTagName){    \
                                    \
                                var table = document.getElementById(id);    \
                                var rows = table.getElementsByTagName(\"tr\");   \
                                for(i = 0; i < rows.length; i++){\   
                                    if(i % 2 == 0){   \
                                        rows[i].className = \"evenrowcolor\";   \
                                    }else{   \
                                        rows[i].className = \"oddrowcolor\";   \
                                    }\ 
                                }   \
                            }   \
                        }   \
                            \
                        window.onload=function(){   \
                            altRows('alternatecolor');   \
                        }   \
                </script>   \
                </head>   \
                <body>   \
                <!-- Table goes in the document BODY -->   \
    <table class=\"altrowstable\" id=\"alternatecolor\">");

    while (true) {
        memset(strfieldname, 0, sizeof(strfieldname));

        // 获取结果集
        if (stmt.next() != 0) break;

        File.Fprintf("<tr>\n");
        for (int i = 0; i < 11; i++) {
            File.Fprintf("<td>%s</td>", strfieldname[i]);
        }
        File.Fprintf("\n");
        File.Fprintf("</tr>\n");
    }


    // 写入尾部信息
    File.Fprintf(" </table> \
                    </body>\
                    </html>");




    return true;
}


bool sendFile(const int sockfd, const char *filename) {

    CFile File;

    File.Open(filename, "r");

    char strBuffer[102400];
    while (true) {
        memset(strBuffer, 0, sizeof(strBuffer));

        if (!File.Fgets(strBuffer, 100000)) break;

        Writen(sockfd, strBuffer, strlen(strBuffer));

    }

    File.Close();

    return true;
}