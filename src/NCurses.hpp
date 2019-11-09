#pragma once
#include "Queue.hpp"
#include "JThread.hpp"
#include <vector>
#include <string>
#include <ncurses.h>
#include "HarpoonEventQueue.hpp"
#include "HackChatEventQueue.hpp"

class BacklogMessage;

class NCurses
{
public:
    NCurses(EventQueue& queue, HackChatEventQueue& hackChatQueue);
    ~NCurses();

    void onInput(const EventInput&);
    void onUserList(const EventUserList&);
    void onUserChanged(const EventUserChanged&);
    void onMessage(const EventMessage&);

private:
    void addMessage(const EventMessage& message);

    EventQueue& queue;
    HackChatEventQueue& hackChatQueue;
    bool redraw;
    bool redrawusers;
    std::mutex usersMutex;
    std::vector<std::string> users;
    std::mutex backlogMutex;
    std::list<BacklogMessage> backlog;
    std::string buffer;
    int lastk = 0;
    NJThread t;
    NJThread eventHandlerThread;
    WINDOW* chatw;
    int scrollOffset;
};
