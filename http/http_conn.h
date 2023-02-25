//
// Created by 欧阳洋 on 2023/2/24.
//

#ifndef HTTP_CONN_H
#define HTTP_CONN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <sys/uio.h>

#include "../lock/locker.h"
class http_conn {
private:
    int m_sockfd;          // http connection's socket fd
    sockaddr_in m_address; // the address of connection
    char  m_read_buf[READ_BUFFER_SIZE]; // 读缓冲区
    int m_read_idx; // 标识读缓冲区中已经读入客户端数据的最后一个字节的下一个位置

public:
    static int m_epollfd;    // 所有的socket上的实际呐都注册到一个全局的epoll中
    static int m_user_count; // 统计用户数量
    static const int READ_BUFFER_SIZE = 2048; // 读缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 2048; // 写缓冲区的大小
    http_conn();
    ~http_conn();
    void process();                                  // 处理客户端的请求， 解析请求报文，并且根据请求的资源响应信息
    void init(int conn_fd, const sockaddr_in &addr); // 初始化
    void close_conn();                               // 关闭连接
    bool read();                                     // 非阻塞地读
    bool write();                                    // 非阻塞地写
};
#endif // HTTP_CONN_H
