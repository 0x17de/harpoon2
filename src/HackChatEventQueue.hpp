#include "EventList.hpp"

using HackChatEvent = EventList_t<
    class EventHackSendMessage,
    class EventHackConnect,
    class EventHackDisconnect,
    class EventHackConnected,
    class EventHackDisconnected
    >;

using HackChatEventQueue = Queue<HackChatEvent>;
