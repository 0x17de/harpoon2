#pragma once
#include "Queue.hpp"
#include <boost/date_time.hpp>
#include "enums/MessageType.hpp"
#include "enums/UserChangeType.hpp"
#include "Queue.hpp"


class EventHackSendMessage
{
public:
    inline EventHackSendMessage(const std::string& message) : message(message) { }

    std::string message;
};
class EventHackConnect
{
public:
    inline EventHackConnect() { }
    inline EventHackConnect(
        std::string server,
        std::string channel,
        std::string username,
        std::string password)
        : server(server)
        , channel(channel)
        , username(username)
        , password(password)
    { }

    std::string server;
    std::string channel;
    std::string username;
    std::string password;
};
class EventHackDisconnect
{
};
class EventHackConnected
{
};
class EventHackDisconnected
{
};

