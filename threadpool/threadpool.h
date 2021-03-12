#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <list>
#include <cstdio>
#include <exception>
#include <pthread.h>
#include "../lock/locker.h"
#include "../mysql/sqlpool.h"

template <typename T>
class threadpool
{
public:
    /*thread_number是线程池中线程的数量，max_requests是请求队列中最多允许的、等待处理的请求的数量*/
    threadpool(connection_pool *connPool, int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T *request);

private:
    /*工作线程运行的函数，它不断从工作队列中取出任务并执行之*/
    static void *worker(void *arg);
    void run();

private:
    int m_thread_number;        //线程池中的线程数
    int m_max_requests;         //请求队列中允许的最大请求数
    pthread_t* m_threads;       //存放线程的数组，其大小为m_thread_number
    std::list<T *> m_queueRequset; //请求队列
    locker m_queuelocker;       //保护请求队列的互斥锁
    sem m_queuesem;            //是否有任务需要处理
    bool m_stop;                //是否结束线程
    connection_pool *m_connPool;  //数据库
};


template <typename T>
threadpool<T>::threadpool( connection_pool *connPool, int thread_number, int max_requests)
{
    m_stop=false;
    m_threads=NULL;
    m_connPool=connPool;
    m_thread_number=thread_number;
    m_max_requests=max_requests;

    if (thread_number <= 0 || max_requests <= 0)
    {
        throw std::exception();
    }
    
    m_threads = new pthread_t[m_thread_number];

    if (!m_threads)
    {
        throw std::exception();
    }
    for (int i = 0; i < thread_number; ++i)
    {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            delete[] m_threads;
            throw std::exception();
        }
        if (pthread_detach(m_threads[i]))//自动释放资源
        {
            delete[] m_threads;
            throw std::exception();
        }
    }
}
template <typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}
template <typename T>
bool threadpool<T>::append(T *request)
{
    m_queuelocker.lock();
    if ( m_queueRequset.size() > m_max_requests)
    {
        m_queuelocker.unlock();
        return false;
    }
    m_queueRequset.push_back(request);//加入请求队列
    m_queuelocker.unlock();
    m_queuesem.post();//发送信号
    return true;
}
template <typename T>
void *threadpool<T>::worker(void *arg)
{
    threadpool *pool = (threadpool *)arg;
    pool->run();
    return pool;
}
template <typename T>
void threadpool<T>::run()
{
    while (!m_stop)
    {
        m_queuesem.wait();
        m_queuelocker.lock();
        if (m_queueRequset.empty())
        {
            m_queuelocker.unlock();
            continue;
        }
        T *request = m_queueRequset.front();//取出请求
        m_queueRequset.pop_front();
        m_queuelocker.unlock();
        if (!request)
            continue;

        connectionRAII mysqlcon(&request->mysql, m_connPool);//从连接池中获取一条连接
        
        request->process();
    }
}
#endif
