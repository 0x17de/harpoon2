#pragma once
#include "EventList.hpp"
#include "Queue.hpp"

using Event = EventList_t<
    class EventInput,
    class EventUserList,
    class EventUserChanged,
    class EventMessage
    >;

using EventQueue = Queue<Event>;
