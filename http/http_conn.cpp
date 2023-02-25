//
// Created by 欧阳洋 on 2023/2/24.
//
#include "http_conn.h"

int http_conn::m_epollfd = -1;      // 所有的socket上的实际都注册到一个全局的epoll中
int http_conn::m_user_count = 0;    // 统计用户数量
const int READ_BUFFER_SIZE = 2048;  // 读缓冲区的大小
const int WRITE_BUFFER_SIZE = 2048; // 写缓冲区的大小
http_conn::http_conn(){};
http_conn::~http_conn(){};
// 设置文件描述符非阻塞
void set_non_blocking(int fd) {
    int old_flag = fcntl(fd, F_GETFL);
    int new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
}
// 边沿触发还是水平触发
// 向epoll中添加需要添加的文件描述符
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    //    event.events = EPOLLIN | EPOLLRDHUP; // 对端断开的事件
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP; // 对端断开的事件

    if (one_shot) {
        event.events |= EPOLLONESHOT;
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    // 设置文件描述符非阻塞
    set_non_blocking(fd);
}
// 从epoll中删除监听的文件描述符
void removefd(int epollfd, int fd) {
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
    init();
}

void http_conn::init() {
    m_check_state = CHECK_STATE_REQUESTLINE; // 正在分析请求行
    m_read_index = 0;
    m_start_line = 0;
    m_checked_index = 0;
    m_url = 0;
    m_version = 0;
    m_method = GET;
    m_linger = false;
    m_content_length = 0;

    bzero(m_read_buf, READ_BUFFER_SIZE);
    bzero(m_write_buf, WRITE_BUFFER_SIZE);
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
    if (m_read_index >= READ_BUFFER_SIZE) {
        return false;
    }

    // 读取到的字节
    int bytes_read = 0;
    // ET模式读，循环地读数据直到无数据可读，或者对方关闭连接
    while (true) {
        bytes_read = recv(m_sockfd, m_read_buf + m_read_index, READ_BUFFER_SIZE - m_read_index, 0);
        if (bytes_read == -1) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                break;
            }
            // 发生错误
            return false;
        } else if (bytes_read == 0) {
            return false; // 对方断开连接
        } else {
            m_read_index += bytes_read;
        }
    }

    printf("读到了数据：%s\n", m_read_buf);
    return true;
}

// 非阻塞地写
bool http_conn::write() {
    printf("一次性写\n");
    return true;
}

// 由线程池中工作线程调用，是处理HTTP请求的入口
void http_conn::process() {
    printf("process start\n");
    // 解析HTTP
    HTTP_CODE read_ret = process_read();
    if (read_ret = NO_REQUEST) {             // 请求不完整，继续获取数据
        modfd(m_epollfd, m_sockfd, EPOLLIN); // 需要重新检测下
        return;
    } else {
    }
    printf("parse the HTTP request, create the response\n");
    // 生成响应
}

// 解析HTTP请求
http_conn::HTTP_CODE http_conn::process_read() {
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST; // the result of request

    char *text = nullptr;

    while (((m_check_state == CHECK_STATE_CONTENT) && line_status == LINE_OK)
           || ((line_status = parse_line()) == LINE_OK)) {
        // 解析到了请求体并且读取完整，或者读取到并且解析了完整的一行

        // 获取一行数据
        text = get_line();

        m_start_line = m_checked_index; // checked_index跟着parse_line（）移动
        printf("get one http line: %s\n", text);
        switch (m_check_state) {
        case CHECK_STATE_REQUESTLINE: {
            ret = parse_request_line(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            }
            break;
        }

        case CHECK_STATE_HEADER: {
            ret = parse_header(text);
            if (ret == BAD_REQUEST) {
                return BAD_REQUEST;
            } else if (ret == GET_REQUEST) {
                do_request();
            }
            break;
        }

        case CHECK_STATE_CONTENT: {
            ret = parse_content(text);
            if (ret == GET_REQUEST) {
                do_request();
            } else {
                line_status = LINE_OPEN;
            }
            break;
        }
        }
    }
    return NO_REQUEST; // 主状态机
};

// 解析HTTP请求首行，获得请求方法，目标URL，HTTP版本
http_conn::HTTP_CODE http_conn::parse_request_line(char *text) {
    // GET index.html HTTP/1.1

    m_url = strpbrk(text, " \t");
    // GET\0index.html HTTP/1.1
    *(m_url++) = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0) {
        m_method = GET;
    } else {
        return BAD_REQUEST;
    }

    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
    // GET\0index.html\0HTTP/1.1
    *(m_version++) = '\0';
    if (strcasecmp(m_version, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }

    // http://192.168.10.1/index.html
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;                 // 192.168.10.1/index.html
        m_url = strchr(m_url, '/'); // /index.html
    }

    if (!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }

    // state transfer to check state header
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

// 解析HTTP请求头
http_conn::HTTP_CODE http_conn::parse_header(char *text) {
    // 遇到空行，表示头部字段解析完毕
    if (text[0] == '\0') {
        // 如果HTTP请求有消息体，则还需要读取m_content_length字节的消息体，
        // 状态机转移到CHECK_STATE_CONTENT状态
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        // 否则说明我们已经得到了一个完整的HTTP请求
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        // 处理Connection 头部字段  Connection: keep-alive
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-Length:", 15) == 0) {
        // 处理Content-Length头部字段
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        // 处理Host头部字段
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        printf("oop! unknow header %s\n", text);
    }
    return NO_REQUEST;
}

// 解析HTTP请求体
http_conn::HTTP_CODE http_conn::parse_content(char *text) {
    if (m_read_index >= (m_content_length + m_checked_index)) {
        text[m_content_length] = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

// 从状态机，根据状态给其他解析器提供行信息
// 解析一行，判断依据\r\n
http_conn::LINE_STATUS http_conn::parse_line() {
    char temp;
    for (; m_checked_index < m_read_index; ++m_checked_index) {
        temp = m_read_buf[m_checked_index];
        if (temp == '\r') {
            if (m_checked_index + 1 == m_read_index) {
                return LINE_OPEN; // 没读完
            } else if (m_checked_index + 1 == '\n') {
                m_read_buf[m_checked_index++] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK; //
            }
            return LINE_BAD;
        } else if (temp == '\n') {
            if (m_checked_index > 1 && (m_read_buf[m_checked_index - 1] == '\r')) {
                m_read_buf[m_checked_index - 1] = '\0';
                m_read_buf[m_checked_index++] = '\0';
                return LINE_OK; //
            }
            return LINE_BAD; // 没读完
        }
    }
    return LINE_OPEN;
}

/* 当解析完所有的request后，得到一个完整的正确的HTTP请求后，分析目标文件的属性
如果目标文件存在、对所有用户可读，且不是目录，则使用mmap将其映射到内存地址m_file_address处
并告诉调用者成功
*/
http_conn::HTTP_CODE http_conn::do_request() {
    return NO_REQUEST;
}