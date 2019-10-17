#pragma once
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <list>
#include <optional>
#include <memory>


template<class T>
class Queue
{
public:
    inline std::optional<T> pop()
    {
        std::unique_lock lock(queueMutex);
        if (queue.empty())
        {
            queueFilled.wait_for(lock, std::chrono::seconds(1));
            if (queue.empty()) return {};
        }
        std::optional res(std::move(queue.front()));
        queue.pop_front();
        return res;
    }

    inline void push(T&& message)
    {
        std::lock_guard lock(queueMutex);
        queue.push_back(message);
        queueFilled.notify_one();
    }

private:
    std::mutex queueMutex;
    std::condition_variable queueFilled;
    std::list<T> queue;
};
