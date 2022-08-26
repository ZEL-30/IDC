# 头文件路径
PUBINCL = -I/project/public

# 库文件路径
PUBLIB = -L/project/public

# 链接库
PUBLIBS = -l_public

# 编译参数
CFLAGS = -w -O2 -g


all: demo01.out demo02.out demo03.out demo04.out demo05.out demo06.out \
	 demo07.out demo08.out demo09.out demo10.out demo11.out demo12.out \
	 demo13.out demo14.out demo15.out demo16.out demo17.out demo18.out \
     demo19.out demo20.out demo21.out demo22.out demo23.out demo24.out \
	 demo25.out demo26.out tcpselect.out tcpclient.out tcppoll.out tcpepoll.out \



demo01.out:demo01.cpp
	g++ $(CFLAGS) demo01.cpp -o demo01.out -lc -lm

demo02.out:demo02.cpp
	g++ $(CFLAGS) demo02.cpp -o demo02.out -lc -lm

demo03.out:demo03.cpp
	g++ $(CFLAGS) demo03.cpp -o demo03.out -lc -lm

demo04.out:demo04.cpp
	g++ $(CFLAGS) demo04.cpp -o demo04.out -lc -lm

demo05.out:demo05.cpp
	g++ $(CFLAGS) demo05.cpp -o demo05.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo06.out:demo06.cpp
	g++ $(CFLAGS) demo06.cpp -o demo06.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo07.out:demo07.cpp
	g++ $(CFLAGS) demo07.cpp -o demo07.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo08.out:demo08.cpp
	g++ $(CFLAGS) demo08.cpp -o demo08.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo09.out:demo09.cpp
	g++ $(CFLAGS) demo09.cpp -o demo09.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo10.out:demo10.cpp
	g++ $(CFLAGS) demo10.cpp -o demo10.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo11.out:demo11.cpp
	g++ $(CFLAGS) demo11.cpp -o demo11.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo12.out:demo12.cpp
	g++ $(CFLAGS) demo12.cpp -o demo12.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo13.out:demo13.cpp
	g++ $(CFLAGS) demo13.cpp -o demo13.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo14.out:demo14.cpp
	g++ $(CFLAGS) demo14.cpp -o demo14.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo15.out:demo15.cpp
	g++ $(CFLAGS) demo15.cpp -o demo15.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo16.out:demo16.cpp
	g++ $(CFLAGS) demo16.cpp -o demo16.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo17.out:demo17.cpp
	g++ $(CFLAGS) demo17.cpp -o demo17.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo18.out:demo18.cpp
	g++ $(CFLAGS) demo18.cpp -o demo18.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo19.out:demo19.cpp
	g++ $(CFLAGS) demo19.cpp -o demo19.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo20.out:demo20.cpp
	g++ $(CFLAGS) demo20.cpp -o demo20.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm

demo21.out:demo21.cpp
	g++ $(CFLAGS) demo21.cpp -o demo21.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo22.out:demo22.cpp
	g++ $(CFLAGS) demo22.cpp -o demo22.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo23.out:demo23.cpp
	g++ $(CFLAGS) demo23.cpp -o demo23.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo24.out:demo24.cpp
	g++ $(CFLAGS) demo24.cpp -o demo24.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo25.out:demo25.cpp
	g++ $(CFLAGS) demo25.cpp -o demo25.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -lc -lm -lpthread

demo26.out:demo26.cpp
	g++ $(CFLAGS) demo26.cpp -o demo26.out $(PUBINCL) $(PUBLIB) $(PUBLIBS) -I/project/public/db/oracle /project/public/db/oracle/_ooci.cpp -I$(ORACLE_HOME)/rdbms/public  -L$(ORACLE_HOME)/lib -L. -lclntsh -lc -lm -lpthread

tcpselect.out:tcpselect.cpp
	g++ $(CFLAGS) tcpselect.cpp -o tcpselect.out -lc -lm

tcpclient.out:tcpclient.cpp
	g++ $(CFLAGS) tcpclient.cpp -o tcpclient.out -lc -lm

tcppoll.out:tcppoll.cpp
	g++ $(CFLAGS) tcppoll.cpp -o tcppoll.out -lc -lm

tcpepoll.out:tcpepoll.cpp
	g++ $(CFLAGS) tcpepoll.cpp -o tcpepoll.out -lc -lm








clean:
	rm -f demo01.out demo02.out demo03.out demo04.out demo05.out demo06.out
	rm -f demo07.out demo08.out demo09.out demo10.out demo11.out demo12.out
	rm -f demo13.out demo14.out demo15.out demo16.out demo17.out demo18.out
	rm -f demo19.out demo20.out demo21.out demo22.out demo23.out demo24.out
	rm -f demo25.out demo26.out 
	rm -f tcpselect.out tcpclient.out tcppoll.out tcpepoll.out
