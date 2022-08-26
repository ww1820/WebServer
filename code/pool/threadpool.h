#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread> // C++ 11 标准库函数
#include <functional>

/*
    生产者-消费者 模型
    生产者：主线程产生任务放入工作队列
    消费者：子线从工作队列取出任务处理

    工作队列（临界区）：线程同步互斥锁
    队列为不为空/满：信号量、条件变量
*/ 

// 线程池类
class ThreadPool {
public:

    // 显式构造函数，线程池大小默认为 8， pool_使用std::make_shared<Pool>()初始化，智能指针初始化最好使用make_shared
    explicit ThreadPool(size_t threadCount = 8): pool_(std::make_shared<Pool>()) {
            assert(threadCount > 0);  // 断言

            // 创建threadCount个子线程
            for(size_t i = 0; i < threadCount; i++) {
                std::thread([pool = pool_] { // 匿名函数，值传递，该匿名函数为子线程的主体函数
                    std::unique_lock<std::mutex> locker(pool->mtx); // 声明独占锁，并上锁
                    while(true) {
                        if(!pool->tasks.empty()) {
                            // 从任务队列中取第一个任务
                            auto task = std::move(pool->tasks.front());
                            // 移除掉队列中第一个元素
                            pool->tasks.pop();
                            locker.unlock();
                            task();  // std::function<void()> 对象，任务主体
                            locker.lock();
                        } 
                        else if(pool->isClosed) break; // 线程池关闭，break while 线程结束
                        else pool->cond.wait(locker);   // 如果队列为空
                        /*
                        The execution of the current thread (which shall have locked lck's mutex) is blocked until notified.
                        At the moment of blocking the thread, the function automatically calls lck.unlock(), allowing other locked threads to continue.
                        */
                    }
                }).detach();// 线程分离，自动回收
            }
    }

    ThreadPool() = default;

    ThreadPool(ThreadPool&&) = default;
    
    ~ThreadPool() { // 析构函数
        if(static_cast<bool>(pool_)) { // 如果pool_不为nullptr，关闭线程池
            {

                // 声明一个 lock_gurad 类型的局部变量保证在销毁或者异常的时候释放锁
                std::lock_guard<std::mutex> locker(pool_->mtx);
                /*using a local lock_guard to lock mtx guarantees unlocking on destruction / exception*/
                pool_->isClosed = true;
            }
            pool_->cond.notify_all(); // 唤醒所有线程
        }
    }

    template<class F>
    void AddTask(F&& task) { // 添加任务，右值引用
        {
            std::lock_guard<std::mutex> locker(pool_->mtx);
            pool_->tasks.emplace(std::forward<F>(task)); // 就地、移动构造
        }
        pool_->cond.notify_one();   // 唤醒一个等待的线程
        /*
        Unblocks one of the threads currently waiting for this condition.
        If no threads are waiting, the function does nothing.
        */
    }

private:
    // 类嵌套类型
    struct Pool {
        std::mutex mtx;                 // 互斥锁
        std::condition_variable cond;   // 条件变量
        bool isClosed;                  // 是否关闭
        std::queue<std::function<void()>> tasks;    // 线程队列（保存的是任务）
    };
    std::shared_ptr<Pool> pool_;  //  智能指针，指向该线程池
};


#endif //THREADPOOL_H