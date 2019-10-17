#pragma once
#include <thread>
#include <mutex>
#include <fstream>

class JThread
{
public:
    template<class... T>
    inline explicit JThread(T... params) : t(std::forward<T>(params)...) {}
    inline ~JThread() { join(); }

    inline JThread(JThread&& other) : t(std::move(other.t)) {}
    inline JThread& operator=(JThread&& other) { t = std::move(other.t); return *this; }
    JThread(const JThread& other) = delete;
    JThread& operator=(const JThread& other) = delete;

    inline void join() { if (t.joinable()) t.join(); }

private:
    std::thread t;
};

class NJThread
{
public:
    inline NJThread()
        : name()
        , threadIndex(0)
    {
    }
    template<class... T>
    inline explicit NJThread(const std::string& name, T... params)
        : t(std::forward<T>(params)...)
        , name(name)
    {
        std::lock_guard lock(mutex);
        threadIndex = ++globalThreadIndex;
#ifdef USE_DEBUGLOG
        std::ofstream("thread.log", std::ios_base::app) << "Starting thread " << name << " #" << threadIndex << '\n';
#endif
    }
    inline ~NJThread() {
        if (threadIndex > 0)
        {
#ifdef USE_DEBUGLOG
            {
                std::lock_guard lock(mutex);
                std::ofstream("thread.log", std::ios_base::app) << "Terminating thread " << name << " #" << threadIndex << '\n';
            }
#endif
            join();
#ifdef USE_DEBUGLOG
            std::lock_guard lock(mutex);
            std::ofstream("thread.log", std::ios_base::app) << "Terminated thread " << name << " #" << threadIndex << '\n';
#endif
        }
    }
    inline NJThread(NJThread&& other)
        : t(std::move(other.t))
        , threadIndex(other.threadIndex)
        , name(std::move(other.name))
    {
        other.threadIndex = 0;
    }
    inline NJThread& operator=(NJThread&& other)
    {
        t = std::move(other.t);
        threadIndex = other.threadIndex;
        name = std::move(other.name);
        other.threadIndex = 0;
        return *this;
    }
    NJThread(const NJThread& other) = delete;
    NJThread& operator=(const NJThread& other) = delete;

    inline void join() { if (t.joinable()) t.join(); }

private:
    static std::mutex mutex;
    static int globalThreadIndex;
    std::thread t;
    int threadIndex;
    std::string name;
};
