# 头文件路径
PUBINCL = -I/project/public
FTPINCL = -I/project/public
MYSQLINCL = -I/usr/include/mysql -I/project/public/db/mysql
ORCINCL = -I$(ORACLE_HOME)/rdbms/public -I/project/public/db/oracle


# 库文件路径
PUBLIB = -L/project/public
FTPLIB = -L/project/public
MYSQLLIB = -L/usr/lib64/mysql
ORCLIB = -L$(ORACLE_HOME)/lib -L.

# 链接库
PUBLIBS = -l_public
FTPLIBS = -lftp
MYSQLLIBS = -lmysqlclient
ORCLIBS = -lclntsh

# CPP文件
FTPCPP =  /project/public/_ftp.cpp
MYSQLCPP = /project/public/db/mysql/_mysql.cpp
TOOLSCPP = /project/tools/c/_tools.cpp
ORCCPP = /project/public/db/oracle/_ooci.cpp

# 编译参数
CFLAGS = -w -O2 -g


all: procctl.out checkproc.out gzipfiles.out deletefiles.out ftpgetfiles.out                    \
	 ftpputfiles.out fileserver.out tcpputfiles.out tcpgetfiles.out execsql.out                 \
	 dminingmysql.out xmltodb.out syncupdate.out syncincrement.out syncincrementex.out          \
	 deletetable.out migratetable.out xmltodb_oracle.out deletetable_oracle.out                 \
	 migratetable_oracle.out dminingoracle.out syncupdate_oracle.out syncincrement_oracle.out   \
	 syncincrementex_oracle.out webserver.out inetd.out rinetd.out rinetdin.out




procctl.out:procctl.cpp
	g++ procctl.cpp -o procctl.out -lc -lm
	cp procctl.out ../bin/procctl

checkproc.out:checkproc.cpp
	g++ $(CFLAGS) checkproc.cpp -o checkproc.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp checkproc.out ../bin/checkproc

gzipfiles.out:gzipfiles.cpp
	g++ $(CFLAGS) gzipfiles.cpp -o gzipfiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp gzipfiles.out ../bin/gzipfiles

deletefiles.out:deletefiles.cpp
	g++ $(CFLAGS) deletefiles.cpp -o deletefiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp deletefiles.out ../bin/deletefiles 

ftpgetfiles.out:ftpgetfiles.cpp
	g++ $(CFLAGS) ftpgetfiles.cpp -o ftpgetfiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(FTPINCL) $(FTPLIB) $(FTPCPP) $(FTPLIBS) -lc -lm
	cp ftpgetfiles.out ../bin/ftpgetfiles

ftpputfiles.out:ftpputfiles.cpp
	g++ $(CFLAGS) ftpputfiles.cpp -o ftpputfiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(FTPINCL) $(FTPLIB) $(FTPCPP) $(FTPLIBS) -lc -lm
	cp ftpputfiles.out ../bin/ftpputfiles

fileserver.out:fileserver.cpp
	g++ $(CFLAGS) fileserver.cpp -o fileserver.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp fileserver.out ../bin/fileserver

tcpputfiles.out:tcpputfiles.cpp
	g++ $(CFLAGS) tcpputfiles.cpp -o tcpputfiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp tcpputfiles.out ../bin/tcpputfiles

tcpgetfiles.out:tcpgetfiles.cpp
	g++ $(CFLAGS) tcpgetfiles.cpp -o tcpgetfiles.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp tcpgetfiles.out ../bin/tcpgetfiles

execsql.out:execsql.cpp
	g++ $(CFLAGS) execsql.cpp -o execsql.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) -lc -lm
	cp execsql.out ../bin/execsql

dminingmysql.out:dminingmysql.cpp
	g++ $(CFLAGS) dminingmysql.cpp -o dminingmysql.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) -lc -lm
	cp dminingmysql.out ../bin/dminingmysql

xmltodb.out:xmltodb.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) xmltodb.cpp -o xmltodb.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) $(TOOLSCPP)  -lc -lm
	cp xmltodb.out ../bin/xmltodb

syncupdate.out:syncupdate.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) syncupdate.cpp -o syncupdate.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) $(TOOLSCPP)  -lc -lm
	cp syncupdate.out ../bin/syncupdate


syncincrement.out:syncincrement.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) syncincrement.cpp -o syncincrement.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) $(TOOLSCPP)  -lc -lm
	cp syncincrement.out ../bin/syncincrement

syncincrementex.out:syncincrementex.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) syncincrementex.cpp -o syncincrementex.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) $(TOOLSCPP)  -lc -lm
	cp syncincrementex.out ../bin/syncincrementex

deletetable.out:deletetable.cpp 
	g++ $(CFLAGS) deletetable.cpp -o deletetable.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) -lc -lm
	cp deletetable.out ../bin/deletetable

migratetable.out:migratetable.cpp _tools.h _tools.cpp
	g++ $(CFLAGS) migratetable.cpp -o migratetable.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) $(TOOLSCPP)  -lc -lm
	cp migratetable.out ../bin/migratetable

xmltodb_oracle.out:xmltodb_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) xmltodb_oracle.cpp _tools_oracle.cpp -o xmltodb_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp xmltodb_oracle.out ../bin/xmltodb_oracle

deletetable_oracle.out:deletetable_oracle.cpp 
	g++ $(CFLAGS) deletetable_oracle.cpp -o deletetable_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp deletetable_oracle.out ../bin/deletetable_oracle

migratetable_oracle.out:migratetable_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) migratetable_oracle.cpp _tools_oracle.cpp -o migratetable_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp migratetable_oracle.out ../bin/migratetable_oracle

dminingoracle.out:dminingoracle.cpp 
	g++ $(CFLAGS) dminingoracle.cpp -o dminingoracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp dminingoracle.out ../bin/dminingoracle

syncupdate_oracle.out:syncupdate_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) syncupdate_oracle.cpp _tools_oracle.cpp -o syncupdate_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp syncupdate_oracle.out ../bin/syncupdate_oracle

syncincrement_oracle.out:syncincrement_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) syncincrement_oracle.cpp _tools_oracle.cpp -o syncincrement_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp syncincrement_oracle.out ../bin/syncincrement_oracle

syncincrementex_oracle.out:syncincrementex_oracle.cpp _tools_oracle.h _tools_oracle.cpp
	g++ $(CFLAGS) syncincrementex_oracle.cpp _tools_oracle.cpp -o syncincrementex_oracle.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm
	cp syncincrementex_oracle.out ../bin/syncincrementex_oracle

webserver.out:webserver.cpp _connpool.h _connpool.cpp
	g++ $(CFLAGS) webserver.cpp _connpool.cpp -o webserver.out  $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(ORCINCL) $(ORCLIB) $(ORCLIBS) $(ORCCPP)  -lc -lm -lpthread
	cp webserver.out ../bin/webserver

inetd.out:inetd.cpp
	g++ $(CFLAGS) inetd.cpp -o inetd.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp inetd.out ../bin/inetd

rinetd.out:rinetd.cpp
	g++ $(CFLAGS) rinetd.cpp -o rinetd.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp rinetd.out ../bin/rinetd

rinetdin.out:rinetdin.cpp
	g++ $(CFLAGS) rinetdin.cpp -o rinetdin.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp rinetdin.out ../bin/rinetdin












clean: 
	rm procctl.out checkproc.out gzipfiles.out deletefiles.out ftpgetfiles.out
	rm ftpputfiles.out fileserver.out tcpputfiles.out tcpgetfiles.out execsql.out
	rm dminingmysql.out xmltodb.out syncupdate.out syncincrement.out syncincrementex.out 
	rm deletetable.out migratetable.out xmltodb_oracle.out deletetable_oracle.out
	rm migratetable_oracle.out dminingoracle.out syncupdate_oracle.out syncincrement_oracle.out
	rm syncincrementex_oracle.out webserver.out inetd.out rinetd.out rinetdin.out