//
// Created by 欧阳洋 on 2023/2/24.
//

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include <cstdio>
#include <exception>

#include "../lock/locker.h"
// 线程池类，定义成模板类为了代码复用，模板参数T为任务类
template <typename T>
class threadpool {
private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    // worker needs to be a static variable. if it is a member function
    // this pointer will be passed to the worker.
    static void *worker(void *arg); // can not visit the non-static members
    void run();

private:
    // 线程的数量
    int m_thread_number;
    // 线程池数组，数量为m_thread_number
    pthread_t *m_threads;
    // 请求队列中允许的最大请求数量
    int m_max_requests;
    // 请求队列
    std::list<T *> m_workqueue;
    // 互斥锁
    locker m_queuelocker;
    // 信号量用来判断是否有任务需要处理
    sem m_queuestat;
    // if exit the thread
    bool m_stop;

public:
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();
    bool append(T *request);
};

template <typename T>
threadpool<T>::threadpool(int thread_number, int max_requests) :
    m_thread_number(thread_number), m_max_requests(max_requests),
    m_stop(false), m_threads(NULL) {
    if ((thread_number <= 0) || (max_requests <= 0)) {
        throw std::exception();
    }
    // 创建threadpool数组
    m_threads = new pthread_t[m_thread_number];
    if (!m_threads) {
        throw std::exception();
    }

    // create m_thread_number threads, set them detached
    for (int i = 0; i < thread_number; ++i) {
        printf("create the %dth thread\n", i);
        // worker needs to be a static variable. if it is a member function
        // this pointer will be passed to the worker.
        if (pthread_create(m_threads + i, NULL, worker, this) != 0) { // pass the parameter this
            delete[] m_threads;
            throw std::exception();
        }

        if (pthread_detach(m_threads[i]) != 0) {
            delete[] m_threads;
            throw std::exception();
        }
    }
}

template <typename T>
threadpool<T>::~threadpool() {
    delete[] m_threads;
    m_stop = true;
}

template <typename T>
bool threadpool<T>::append(T *request) {
    m_queuelocker.lock();
    // if the work queue is bigger than the max number, it should not append
    if (m_workqueue.size() >= m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }
    // if queue is not full, append it
    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    // semaphore increment
    m_queuestat.post();
    return true;
}

template <typename T>
void *threadpool<T>::worker(void *arg) {
    // pass the parameter this into this function
    threadpool *pool = (threadpool *)arg;
    // 从工作队列中取数据
    pool->run();
    return pool;
}

template <typename T>
void threadpool<T>::run() {
    // stop until m_stop is not equal to false
    while (!m_stop) {
        m_queuestat.wait();   // wait for the increment of semaphore
        m_queuelocker.lock(); // lock to operate
        if (m_workqueue.empty()) {
            m_queuelocker.unlock();
            continue;
        }
        // it will be an object
        T *request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();
        if (!request) {
            continue;
        }
        request->process();
    }
}
#endif // THREADPOOL_H