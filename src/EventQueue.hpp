#include "EventBase.hpp"
#include "Queue.hpp"

using EventQueue = Queue<std::shared_ptr<EventBase>>;
