//
// Created by 欧阳洋 on 2023/2/24.
//

#include "lock/locker.h"
#include "threadpool/threadpool.h"

#include <string>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <libgen.h>

// 添加信号捕捉
// 这里也可以直接写函数类型，而不是函数指针，函数类型会自动地转换成指向函数的指针
void addsig(int sig, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

int main(int argc, char* argv[]) {
    if(argc <= 1) {
        // basename return the
        printf("Please run program as the format: %s <port_number>\n", basename(argv[0]));
        exit(-1);
    }
    // 获取端口号
    int port = atoi(argv[1]);

    // 处理SIGPIPE做处理, will terminate the process in default,
    // 如果不捕捉SIGPIPE信号，怎么样才能测试出bug
    addsig(SIGPIPE, SIG_IGN);

    threadpool<http_conn> *pool = NULL;
    try {
        pool = new threadpool<http_conn>;
    } catch(...) {
        exit(-1);
    }
    return EXIT_SUCCESS;
}