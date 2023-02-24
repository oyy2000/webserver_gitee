//
// Created by 欧阳洋 on 2023/2/22.
//

#ifndef LOCKER_H
#define LOCKER_H
// 线程同步机制封装类
// 互斥锁类
#include<pthread.h>
#include<exception>
#include<semaphore.h>

class locker {
private:
    pthread_mutex_t m_mutex;
public:
    locker() {
        // 生成失败
        if(pthread_mutex_init(&m_mutex, NULL) != 0) {
            throw std::exception();
        }
    }

    ~locker() {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() {
        return pthread_mutex_lock(&m_mutex);
    }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex);
    }

    // 获取互斥变量
    pthread_mutex_t *get() {
        return &m_mutex;
    }
};

// 条件变量
class cond{
private:
    pthread_cond_t m_cond;
public:
    cond() {
        if(pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }

    ~cond() {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t *m_mutex) {
        return pthread_cond_wait(&m_cond, m_mutex) == 0;
    }

    bool timedwait(pthread_mutex_t *m_mutex, struct timespec t) {
        return pthread_cond_timedwait(&m_cond, m_mutex, &t) == 0;
    }
    // 通知一个变量唤醒
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }
    // 通知所有的唤醒
    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }
};

// 信号量类
class sem {
private:
    sem_t m_sem;
public:
    sem() {
        if(sem_init(&m_sem, 0 , 0) != 0) {
            throw std::exception();
        }
    }

    sem(int num) {
        if(sem_init(&m_sem, 0 , num) != 0) {
            throw std::exception();
        }
    }

    ~sem() {
        sem_destroy(&m_sem);
    }

    bool wait() {
        //等待信号量
        return sem_wait(&m_sem) == 0;
    }

    bool post() {
        return sem_post(&m_sem) == 0;
    }
};







#endif //LOCKER_H
