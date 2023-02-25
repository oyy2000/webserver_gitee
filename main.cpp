//
// Created by 欧阳洋 on 2023/2/24.
//

#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/epoll.h>
#include <libgen.h>

#include "lock/locker.h"
#include "threadpool/threadpool.h"
#include "http/http_conn.h"
#define MAX_FD 65535 // 最大文件描述符个数
#define MAX_EPOLL_EVENT_NUMBER 10000

// 添加信号捕捉
// 这里也可以直接写函数类型，而不是函数指针，函数类型会自动地转换成指向函数的指针
void addsig(int sig, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    sigaction(sig, &sa, NULL);
}

// 添加文件描述符到epoll——event
extern void addfd(int epollfd, int fd, bool one_shot);
// 从epoll中删除文件描述符
extern void removefd(int epollfd, int fd);
// 修改描述符，重置ONESHOT
extern void modfd(int epollfd, int fd, int ev);

int main(int argc, char *argv[]) {
    if (argc <= 1) {
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
    } catch (...) {
        exit(-1);
    }

    // 创建一个数组，保存所有的客户端信息
    http_conn *users = new http_conn[MAX_FD];

    int listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    // 设置端口复用功能
    int reuse = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // 绑定
    struct sockaddr_in address;
    address.sin_family = PF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    bind(listen_fd, (struct sockaddr *)&address, sizeof(address));

    // 监听
    listen(listen_fd, 5);

    // 创建epoll对象， 事件数组，添加
    epoll_event events[MAX_EPOLL_EVENT_NUMBER];
    int epollfd = epoll_create(5);

    // 将文件描述符添加到epoll当中
    addfd(epollfd, listen_fd, false);
    http_conn::m_epollfd = epollfd;

    while (true) {
        int num = epoll_wait(epollfd, events, MAX_EPOLL_EVENT_NUMBER, -1); // the number of readable events
        if ((num < 0) && (errno != EINTR)) {
            printf("epoll failure\n");
            break;
        }

        // 循环遍历事件数组
        for (int i = 0; i < num; i++) {
            int sockfd = events[i].data.fd;
            if (sockfd == listen_fd) {
                // 有客户端连接进来
                sockaddr_in client_addr;
                socklen_t client_addrlen = sizeof(client_addr);
                int conn_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &client_addrlen);

                if (http_conn::m_user_count >= MAX_FD) {
                    // 目前的连接数满了
                    // response the information that the server is busy with so many connections now.
                    close(sockfd);
                    continue;
                }

                // 新的客户的数据初始化，放到user数组中
                // 将连接的文件描述符作为
                users[conn_fd].init(conn_fd, client_addr);
            } else if (events[i].events & (EPOLLHUP | EPOLLRDHUP | EPOLLERR)) {
                // 对方异常断开或者错误
                users[sockfd].close_conn();
            } else if (events[i].events & EPOLLIN) { // 读到数据
                // 一次性把数据都读完
                if (users[sockfd].read() != 0) {
                    pool->append(&users[sockfd]);
                } else {
                    // 读失败了
                    users[sockfd].close_conn();
                }
            } else if (events[i].events & EPOLLOUT) { // 有写的数据
                // 一次性写完
                if (users[sockfd].write() == 0) {
                    // 写失败了
                    users[sockfd].close_conn();
                }
            }
        }
    }
    close(epollfd);
    close(listen_fd);
    delete[] users;
    delete pool;

    return EXIT_SUCCESS;
}