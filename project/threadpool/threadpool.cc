#include "threadpool.h"

threadpool::threadpool(int num):stop(false)
{
    for(int i = 0; i < num; ++i)
    {
        workers.emplace_back([this]{
                while(true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> lock(queue_mutex);
                        condition.wait(lock, [this]{return stop | !tasks.empty();});
                        if(stop && tasks.empty())
                            return;
                        task = std::move(tasks.front());
                        tasks.pop();
                    }

                    task();
                }
                });
    }
}

threadpool::~threadpool()
{
    stop = true;
    condition.notify_all();

    for(auto& worker : workers)
       worker.join();
} 
