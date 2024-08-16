#include "headers.h"

// class ThreadPool {
// public:
//     ThreadPool(size_t numThreads);
//     ~ThreadPool();

//     void enqueueTask(std::function<void()> task);

// private:
//     std::vector<std::thread> workers;
//     std::queue<std::function<void()>> tasks;

//     std::mutex queueMutex;
//     std::condition_variable condition;
//     std::atomic<bool> stop;

//     void workerThread();
// };

ThreadPool::ThreadPool(size_t numThreads) : stop(false), activeTasks(0) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] {
                        return this->stop || !this->tasks.empty();
                    });

                    if (this->stop && this->tasks.empty()) return;

                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }

                // Execute the task and track the active task count
                activeTasks++;
                task();
                activeTasks--;

                // Notify that a task has completed
                waitCondition.notify_one();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop = true;
    }
    condition.notify_all();
    for (std::thread &worker : workers) worker.join();
}

void ThreadPool::enqueueTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
        tasks.emplace(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::wait() {
    std::unique_lock<std::mutex> lock(queueMutex);
    waitCondition.wait(lock, [this] { return tasks.empty() && activeTasks == 0; });
}
