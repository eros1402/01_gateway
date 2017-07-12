CFLAGS = -Wall -c
LFLAGS = -Wall
DFLAGS = -DDEBUG
GFLAGS = -D_GNU_SOURCE
PFLAGS = -pthread
MYSQLCFLAGS = -I/usr/include/mysql -DBIG_JOINS=1 -fno-strict-aliasing -g
MYSQLLIBS = -L/usr/lib/arm-linux-gnueabihf -lmysqlclient -lz -lm -lrt -ldl
SQLITE3 = -lsqlite3
LIB_PATH = -L/home/chi/Desktop
MONGOOSE = -lmongoose
MYLIST = -llist
MYQUEUE = -lmyqueue
TCPSOCK = -ltcpsocket
DATAMGR = -ldatamgr
CC = gcc
OBJS = sensor_db.o datamgr.o err_handler.o mylocalfunc.o sensor_sqlite3db.o
EXE = main
	
all: main.c sensor_db.o err_handler.o mylocalfunc.o sensor_sqlite3db.o	
	$(CC) $(GFLAGS) $(DFLAGS) $(LFLAGS) main.c $(OBJS) -o $(EXE) $(PFLAGS) $(MYSQLCFLAGS) $(MYSQLLIBS) $(SQLITE3) $(LIB_PATH) $(MONGOOSE) $(MYLIST) $(MYQUEUE) $(TCPSOCK) $(DATAMGR)
	valgrind --tool=memcheck --leak-check=yes ./$(EXE)

sensor_sqlite3db.o: sensor_sqlite3db.h sensor_sqlite3db.c	
	$(CC) $(CFLAGS) sensor_sqlite3db.c $(SQLITE3)
	
mylocalfunc.o: mylocalfunc.h mylocalfunc.c
	$(CC) $(DFLAGS) $(CFLAGS) mylocalfunc.c $(LIB_PATH) $(MONGOOSE)

sensor_db.o: sensor_db.h sensor_db.c
	$(CC) $(DFLAGS) $(CFLAGS) $(GFLAGS) sensor_db.c $(MYSQLCFLAGS) $(MYSQLLIBS)

datamgr.o: datamgr.h datamgr.c
	$(CC) $(CFLAGS) datamgr.c
	
err_handler.o: err_handler.h err_handler.c
	$(CC) $(CFLAGS) err_handler.c

clean:
	rm -f *~ *.o core
