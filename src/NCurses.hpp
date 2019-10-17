#pragma once
#include "Queue.hpp"
#include "JThread.hpp"
#include <vector>
#include <string>
#include <ncurses.h>
#include "EventQueue.hpp"

class BacklogMessage;

class NCurses
{
public:
    NCurses(EventQueue& queue, EventQueue& hackChatQueue);
    ~NCurses();

    void onInput(class EventInput&);
    void onUserList(class EventUserList&);
    void onUserChanged(class EventUserChanged&);
    void onMessage(class EventMessage&);

private:
    void addMessage(const EventMessage& message);

    EventQueue& queue;
    EventQueue& hackChatQueue;
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
