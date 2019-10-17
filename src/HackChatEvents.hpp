#pragma once
#include "Queue.hpp"
#include <boost/date_time.hpp>
#include "EventBase.hpp"
#include "HackChatClient.hpp"
#include "enums/MessageType.hpp"
#include "enums/UserChangeType.hpp"


EVENT_CLASS(hackchat::Client, HackSendMessage)
{
public:
    inline EventHackSendMessage(const std::string& message) : message(message) { }

    std::string message;
};
EVENT_CLASS(hackchat::Client, HackConnect)
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
EVENT_CLASS(hackchat::Client, HackDisconnect)
{
};
EVENT_CLASS(hackchat::Client, HackConnected)
{
};
EVENT_CLASS(hackchat::Client, HackDisconnected)
{
};
