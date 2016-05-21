all:
	g++ -fpermissive -I ../include/ -I /usr/include/mysql/ -lpthread -ggdb -c dvrSql.cpp
	g++ -fpermissive -I ../include/ -I /usr/include/mysql/ -lpthread -I /usr/include/libxml2/ -ggdb -c confXmlReader.cpp
	g++ -fpermissive -I ../include/ -I /usr/include/mysql/ -lpthread -ggdb -c dvrMain.cpp
	g++ confXmlReader.o  dvrMain.o  dvrSql.o -Lstatic -L/usr/lib/i386-linux-gnu -lmysqlclient -lpthread -lz -lm -ldl -lpthread -lxmnetsdk -lxml2 -ggdb -o dvr_daemon
clean:
	rm -f *.o
	rm -f dvr_daemon
