
#pragma once
#include <pthread.h>
#include <signal.h>
#include "globals.hpp"

class SimpleSignalHandler
{
public:
    inline SimpleSignalHandler()
    {
        signal(SIGPIPE, SIG_IGN);
        sigemptyset(&sigset);
        sigaddset(&sigset, SIGTERM);
        sigaddset(&sigset, SIGINT);
        if (sigprocmask(SIG_BLOCK, &sigset, NULL)) throw std::runtime_error("Failed to set signal handler");
    }
    inline void handle()
    {
        timespec ts;
        ts.tv_sec = 1;
        ts.tv_nsec = 0;
        while (RUNNING)
        {
            int sig = sigtimedwait(&sigset, 0, &ts);
            if (sig == SIGTERM || sig == SIGINT) break;
        }
        RUNNING = false;
    }

private:
    sigset_t sigset;
};
