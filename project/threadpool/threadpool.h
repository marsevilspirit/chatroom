#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <future>
#include <queue>
#include <thread>
#include <mutex>
#include <vector>
#include <functional>
#include <condition_variable>

#define THREAD_NUM 20

class threadpool {
public:
    threadpool(size_t threads);
    ~threadpool();

    template <typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable if_tasks_pop;
    bool stop;
};

template<class F, class... Args>
auto threadpool::enqueue(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    if_tasks_pop.notify_one();
    return res;
}

#endif // THREADPOOL_H
