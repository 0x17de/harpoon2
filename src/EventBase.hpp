#pragma once
#include <variant>

template<class Target, class EventType>
struct EventTraits { };

template<class Target, class EventType>
inline void handleEvent(Target& target, const EventType& event)
{
    std::visit(
        [&target](auto&& event)
        {
            (target.*EventTraits<Target, std::decay_t<decltype(event)>>::Callback)
                    (event);
        }, event);
}

#define MAP_EVENT_CB(TARGET, EVENT_NAME, TARGET_CB)                 \
    template<>                                                      \
    struct EventTraits<TARGET, Event ## EVENT_NAME>                 \
    {                                                               \
        constexpr static decltype(TARGET_CB) Callback = TARGET_CB;  \
    }
#define MAP_EVENT(TARGET, EVENT_NAME)                           \
    MAP_EVENT_CB(TARGET, EVENT_NAME, &TARGET::on ## EVENT_NAME)
