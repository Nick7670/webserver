server: main.c ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./mysql/sqlpool.cpp ./mysql/sqlpool.h
	g++ -w -g -o server main.c ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./lock/locker.h ./mysql/sqlpool.cpp ./mysql/sqlpool.h -lpthread -lmysqlclient


clean:
	rm  -r server
