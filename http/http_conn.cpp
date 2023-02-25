//
// Created by 欧阳洋 on 2023/2/24.
//
#include "http_conn.h"

int http_conn::m_epollfd = -1;   // 所有的socket上的实际都注册到一个全局的epoll中
int http_conn::m_user_count = 0; // 统计用户数量

http_conn::http_conn(){};
http_conn::~http_conn(){};
// 设置文件描述符非阻塞
int set_non_blocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}
// 边沿触发还是水平触发
// 向epoll中添加需要添加的文件描述符
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP; // 对端断开的事件

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞
    set_non_blocking(fd);
}
// 从epoll中删除监听的文件描述符
int removefd(int epollfd, int fd) {
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
    close(fd);
}
// 修改文件描述符，重置ONESHOT事件，以确保下次socket上有可读事件时，能被触发
void modfd(int epollfd, int fd, int ev) {
    epoll_event event;
    event.data.fd = fd;
    event.events = ev | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}
// initialize the http connection
void http_conn::init(int sockfd, const sockaddr_in &addr) {
    m_sockfd = sockfd;
    m_address = addr;

    // 设置端口复用功能
    int reuse = 1;
    setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    // add it into epoll
    addfd(m_epollfd, sockfd, true);
    // 总用户数加1
    m_user_count++;
}
// close the connection
void http_conn::close_conn() {
    if (m_sockfd != -1) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        m_user_count--;
    }
}

// 非阻塞地读
bool http_conn::read() {
    printf("一次性读\n");
}

// 非阻塞地写
bool http_conn::write() {
    printf("一次性写\n");
}

// 由线程池中工作线程调用，是处理HTTP请求的入口
void http_conn::process() {
    // 解析HTTP
    printf("parse the HTTP request, create the response");
    // 生成响应
}