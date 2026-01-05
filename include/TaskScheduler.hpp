#pragma once
#include <vector>
#include <thread>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>

class TaskScheduler {
public:
    TaskScheduler(size_t threadCount = std::thread::hardware_concurrency());
    ~TaskScheduler();

    void enqueue(std::function<void()> task);
    void wait(); // Wait for all current tasks to complete

private:
    std::vector<std::thread> m_workers;
    std::queue<std::function<void()>> m_tasks;
    std::mutex m_queueMutex;
    std::condition_variable m_condition;
    std::condition_variable m_waitCondition;
    std::atomic<bool> m_stop;
    std::atomic<int> m_activeTasks;
};
