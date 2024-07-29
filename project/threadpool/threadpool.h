#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <thread>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>

#define THREAD_NUM 10

class threadpool{
public:
    threadpool(int num);


    template<typename T, typename... Args>
    void enqueue(T&& t, Args&&... args)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::bind(std::forward<T>(t),std::forward<Args>(args)...));
        }

        condition.notify_one();
    }


    ~threadpool();

private:
    bool stop{false};
    std::queue<std::function<void()>> tasks{};
    std::vector<std::thread> workers{};
    std::condition_variable condition{};
    std::mutex queue_mutex{};
};

#endif // THREADPOOL_H
