2.24
- basename() return the last component of the path
- SIGPIPE 的捕捉
- EPOLLONESHOT 和 EPOLLRDHUP？
```c++
void addfd(int epollfd, int fd, bool one_shot) {
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLRDHUP; // 对端断开的事件
}
ONESHOT
一个socket上的事件可能被多次触发。
对于注册了EPOLLONESHOT事件的文件描述符，操作系统上最多触发其上注册的一个可读、可写或者异常事件，且只触发一次
除非用epoll_ctl 重置该文件描述符上注册的EPOLLONESHOT事件
一旦处理完毕，需要重置EPOLLONESHOT事件
```



```c
epoll wait 为什么要检测EINTR
int num = epoll_wait(epollfd, events, MAX_EPOLL_EVENT_NUMBER, -1) // the number of readable events
        if ((num < 0) && (errno != EINTR)) {
```



- 新的客户的数据初始化，放到http_conn静态的user数组中，conn_fd作为索引
  - Init内容为conn_fd and client address
