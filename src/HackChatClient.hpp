#pragma once
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <json/json.h>
#include "JThread.hpp"
#include "EventQueue.hpp"
#include "globals.hpp"

class EventHackSendMessage;
class EventHackConnect;
class EventHackConnected;
class EventHackDisconnect;
class EventHackDisconnected;

namespace hackchat
{

using WssClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WssMessagePtr = websocketpp::config::asio_client::message_type::ptr;
using WssErrorCode = websocketpp::lib::error_code;

class Client
{
public:
    Client(EventQueue& harpoon);
    ~Client();

    void onHackSendMessage(EventHackSendMessage& event);
    void onHackConnect(EventHackConnect& event);
    void onHackConnected(EventHackConnected& event);
    void onHackDisconnect(EventHackDisconnect& event);
    void onHackDisconnected(EventHackDisconnected& event);

    EventQueue queue;

private:
    EventQueue& harpoon;

    std::string server, channel, username, password;

    bool connected;
    WssClient wss;
    websocketpp::connection_hdl wssHandle;
    NJThread wssThread;
    NJThread wssPingThread;
    std::mutex wssPingMutex;
    std::condition_variable wssPingCondition;

    NJThread eventHandlerThread;
};

}
