
半同步/半反应堆线程池
===============
使用一个工作队列完全解除了主线程和工作线程的耦合关系：
- 主线程往工作队列中插入任务，工作线程通过竞争(工作线程一直while循环)来取得任务并执行它，并且通过信号量进行同步。
> * 同步I/O模拟proactor模式
> * 半同步/半反应堆
> * 线程池


Debug 日志
```c
if (pthread_create(m_threads + i, NULL, worker, this) != 0) { // 这里要传参数this
            delete[] m_threads;
            throw std::exception();
        }
```




