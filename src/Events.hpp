#pragma once
#include "Queue.hpp"
#include <boost/date_time.hpp>
#include "EventBase.hpp"
#include "NCurses.hpp"
#include "enums/MessageType.hpp"
#include "enums/UserChangeType.hpp"


EVENT_CLASS(NCurses, Input)
{
public:
    inline EventInput(const std::string& message) : message(message) {}
    inline virtual ~EventInput() = default;

    std::string message;
};
EVENT_CLASS(NCurses, UserList)
{
public:
    inline EventUserList(std::vector<std::string>&& users)
        : users(users)
    {
    }
    inline EventUserList(const std::vector<std::string>& users)
        : users(users.begin(), users.end())
    {
    }

    std::vector<std::string> users;
};
EVENT_CLASS(NCurses, UserChanged)
{
public:
    inline EventUserChanged(std::string&& user, UserChangeType changeType)
        : user(user)
        , changeType(changeType)
    {
    }
    inline EventUserChanged(const std::string& user, UserChangeType changeType)
        : user(user)
        , changeType(changeType)
    {
    }

    std::string user;
    UserChangeType changeType;
};
EVENT_CLASS(NCurses, Message)
{
public:
    inline EventMessage(const std::string& sender,
                        const std::string& message,
                        MessageType type = MessageType::Normal)
        : time(boost::posix_time::second_clock::universal_time())
        , sender(sender)
        , message(message)
        , type(type)
    {
    }

    boost::posix_time::ptime time;
    std::string sender;
    std::string message;
    MessageType type;
    std::map<std::string, std::string> extra;
};

