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


void ThreadPool::workerThread() {
    while (!stop) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queueMutex);

            condition.wait(lock, [this] { return stop || !tasks.empty(); });

            if (stop && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }
        try{
            task();
        }
        catch(const std::exception& e){
            cout << "ERROR AT THREAD!!!\n" << flush;
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] { this->workerThread(); });
    }
}

void ThreadPool::enqueueTask(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}


ThreadPool::~ThreadPool() {
    {
    std::unique_lock<std::mutex> lock(queueMutex);
    stop = true;
    }
    condition.notify_all();

    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}
