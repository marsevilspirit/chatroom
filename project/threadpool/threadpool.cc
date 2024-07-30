#include "threadpool.h"

threadpool::threadpool(size_t threads) : stop(false) {
    for(int i = 0; i < threads; i++)
    {
        workers.emplace_back([this]{
                while(true)
                {
                    std::function<void()> task;

                    {
                        std::unique_lock<std::mutex> queue_lock(this->queue_mutex);
                        this->if_tasks_pop.wait(queue_lock, [this]{return this->stop || !this->tasks.empty();});
                        if(this->stop && this->tasks.empty())
                            return;

                        task = std::move(this->tasks.front());
                        tasks.pop();
                    }

                    task();
                }
                });        
    }
}

threadpool::~threadpool() {
    {
        std::unique_lock<std::mutex> queue_lock(queue_mutex);
        stop = true;
    }

    if_tasks_pop.notify_all();
    for(std::thread &worker : workers)
    {
        worker.join();
    }
}
