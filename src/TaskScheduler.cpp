#include "TaskScheduler.hpp"

TaskScheduler::TaskScheduler(size_t threadCount) : m_stop(false), m_activeTasks(0) {
    for (size_t i = 0; i < threadCount; ++i) {
        m_workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(m_queueMutex);
                    m_condition.wait(lock, [this] { return m_stop || !m_tasks.empty(); });
                    if (m_stop && m_tasks.empty()) return;
                    task = std::move(m_tasks.front());
                    m_tasks.pop();
                }
                task();
                m_activeTasks--;
                m_waitCondition.notify_all();
            }
        });
    }
}

TaskScheduler::~TaskScheduler() {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_stop = true;
    }
    m_condition.notify_all();
    for (std::thread& worker : m_workers) {
        worker.join();
    }
}

void TaskScheduler::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(m_queueMutex);
        m_activeTasks++;
        m_tasks.emplace(std::move(task));
    }
    m_condition.notify_one();
}

void TaskScheduler::wait() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_waitCondition.wait(lock, [this] { return m_tasks.empty() && m_activeTasks == 0; });
}
