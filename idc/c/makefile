# 头文件路径
PUBINCL = -I/project/public
MYSQLINCL = -I/usr/include/mysql -I/project/public/db/mysql

# 库文件路径
PUBLIB = -L/project/public
MYSQLLIB = -L/usr/lib64/mysql

# CPP文件
MYSQLCPP = /project/public/db/mysql/_mysql.cpp

# 链接库
PUBLIBS = -l_public
MYSQLLIBS = -lmysqlclient

# 编译参数
CFLAGS = -w -O2 -g


all: crtsurfdata.out obtcodetodb.out obtmindtodb.out





crtsurfdata.out:crtsurfdata.cpp
	g++ $(CFLAGS) crtsurfdata.cpp -o crtsurfdata.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm
	cp crtsurfdata.out ../bin/crtsurfdata

obtcodetodb.out:obtcodetodb.cpp
	g++ $(CFLAGS) obtcodetodb.cpp -o obtcodetodb.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) -lc -lm
	cp obtcodetodb.out ../bin/obtcodetodb

obtmindtodb.out:obtmindtodb.cpp idcapp.h idcapp.cpp
	g++ $(CFLAGS) obtmindtodb.cpp -o obtmindtodb.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLCPP) $(MYSQLLIBS) idcapp.cpp -lc -lm
	cp obtmindtodb.out ../bin/obtmindtodb


clean:
	rm -f crtsurfdata.out obtcodetodb.out obtmindtodb.out
