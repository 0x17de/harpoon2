#pragma once
#include <websocketpp/config/asio_client.hpp>
#include <websocketpp/client.hpp>
#include <json/json.h>
#include "JThread.hpp"
#include "HarpoonEventQueue.hpp"
#include "HackChatEventQueue.hpp"
#include "globals.hpp"

namespace hackchat
{

using WssClient = websocketpp::client<websocketpp::config::asio_tls_client>;
using WssMessagePtr = websocketpp::config::asio_client::message_type::ptr;
using WssErrorCode = websocketpp::lib::error_code;

class Client
{
public:
    explicit Client(EventQueue& harpoon);
    ~Client();

    void onHackSendMessage(const EventHackSendMessage& event);
    void onHackConnect(const EventHackConnect& event);
    void onHackConnected(const EventHackConnected& event);
    void onHackDisconnect(const EventHackDisconnect& event);
    void onHackDisconnected(const EventHackDisconnected& event);

    HackChatEventQueue queue;

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
