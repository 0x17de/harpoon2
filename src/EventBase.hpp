#pragma once
#include <type_traits>

#define EVENT_CLASS(Target, CallbackName)        \
    class Event ## CallbackName : public ::Event<Target, Event ## CallbackName, &Target::on ## CallbackName>

class EventBase
{
public:
    inline virtual ~EventBase() = default;
    virtual void handle(void* t) = 0;
    template<class T>
    T& as() { return *static_cast<T*>(this); }
};

template<class Target, class EventTarget, void(Target::*Callback)(EventTarget&)>
class Event : public EventBase
{
public:
    Event() { }
    virtual ~Event() { }

    virtual void handle(void* t)
    {
        (static_cast<Target*>(t)->*Callback)(*static_cast<EventTarget*>(this));
    }
};
