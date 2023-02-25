CXX ?= g++

DEBUG ?= 1
ifeq ($(DEBUG), 1)
    CXXFLAGS += -g
else
    CXXFLAGS += -O2

endif

server: main.cpp  ./http/http_conn.cpp #./timer/lst_timer.cpp ./log/log.cpp ./CGImysql/sql_connection_pool.cpp  webserver.cpp config.cpp # -lmysqlclient
	$(CXX) -o server  $^ $(CXXFLAGS) -lpthread  

clean:
	rm  -r server
