#include "HackChatClient.hpp"
#include <boost/date_time.hpp>
#include "HackChatEvents.hpp"
#include "Events.hpp"

namespace hackchat
{


static boost::posix_time::ptime parseTime(boost::posix_time::ptime& result, const Json::Value& timeValue)
{
    if (timeValue.isIntegral())
    {
        try
        {
            std::time_t time = timeValue.asInt64() / 1000;
            return boost::posix_time::from_time_t(time);
        }
        catch(...) { } // ignore otherwise
    }
    return boost::posix_time::ptime();
}


Client::Client(EventQueue& harpoon)
    : harpoon(harpoon)
{
    wss.init_asio();
    wss.start_perpetual();
    wss.clear_access_channels(websocketpp::log::alevel::all);

    eventHandlerThread = NJThread(
        "chatEventHandler",
        [this]
        {
            while (RUNNING)
            {
                if (const auto& optMessage = this->queue.pop())
                    (*optMessage)->handle(this);
            }
        });
}
Client::~Client()
{
    wss.stop_perpetual();
    wss.stop();
    connected = false;
    std::lock_guard lock(wssPingMutex);
    wssPingCondition.notify_all();
}

void Client::onHackSendMessage(EventHackSendMessage& event)
{
    Json::Value root;
    root["cmd"] = "chat";
    root["text"] = event.message;

    WssErrorCode ec;
    wss.send(
        wssHandle,
        Json::writeString(Json::StreamWriterBuilder(), root),
        websocketpp::frame::opcode::text,
        ec);
}
void Client::onHackConnect(EventHackConnect& event)
{
    server = event.server;
    channel = event.channel;
    username = event.username;
    password = event.password;

    harpoon.push(
        std::make_shared<EventMessage>(
            "system",
            "connecting to hack.chat " + server,
            MessageType::Status));
    connected = true;
    wss.set_tls_init_handler(
        [](auto hdl)
        {
            auto ctx = websocketpp::lib::make_shared<boost::asio::ssl::context>(boost::asio::ssl::context::tls);
            ctx->set_options(boost::asio::ssl::context::default_workarounds |
                             boost::asio::ssl::context::no_sslv2 |
                             boost::asio::ssl::context::no_sslv3 |
                             boost::asio::ssl::context::single_dh_use);
            return ctx;
        });
    wss.set_message_handler(
        [this](auto hdl, WssMessagePtr msg)
        {
            const std::string& payload = msg->get_payload();

            try
            {
                Json::CharReaderBuilder builder;
                const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
                Json::Value root;
                JSONCPP_STRING err;
                if (!reader->parse(payload.c_str(), payload.c_str()+payload.size(), &root, &err)) return; // skip

#ifdef USE_DEBUGLOG
                std::ofstream("hack.log", std::ios_base::app) << payload << '\n';
#endif
                const std::string& cmd = root.get("cmd", "").asString();

                if (cmd == "chat")
                {
                    std::string text = root.get("text", "").asString();
                    text.erase(std::remove(text.begin(), text.end(), '\0'), text.end()); // remove nullbytes

                    auto event = std::make_shared<EventMessage>(
                        root.get("nick", "system").asString(),
                        text);
                    parseTime(event->time, root["time"]);

                    event->extra["trip"] = root.get("trip", "").asString();
                    event->extra["mod"] = root.get("mod", false).asBool() ? "1" : "";

                    harpoon.push(std::move(event));
                }
                else if (cmd == "warn")
                {
                    auto event = std::make_shared<EventMessage>(
                        "system",
                        root.get("text", "").asString(),
                        MessageType::Status);
                    parseTime(event->time, root["time"]);
                    harpoon.push(std::move(event));
                }
                else if (cmd == "info")
                {
                    const std::string& type = root.get("type", "").asString();
                    if (type == "whisper")
                    {
                        const std::string& nick = root.get("from", "").asString();
                        const std::string& trip = root.get("trip", "").asString();
                        const std::string& utype = root.get("utype", "").asString();
                        auto event = std::make_shared<EventMessage>(
                            nick,
                            root.get("text", "").asString(),
                            MessageType::Whisper);
                        if (!trip.empty()) event->extra["trip"] = trip;
                        if (utype == "mod") event->extra["mod"] = "1";
                        harpoon.push(std::move(event));
                    }
                    else if (type == "emote")
                    {
                        const std::string& nick = root.get("nick", "").asString();
                        harpoon.push(
                            std::make_shared<EventMessage>(nick,
                                                           root.get("text", "").asString(),
                                                           MessageType::Me));
                    }
                }
                else if (cmd == "onlineAdd")
                {
                    const std::string& nick = root.get("nick", "").asString();
                    harpoon.push(
                        std::make_shared<EventUserChanged>(nick, UserChangeType::Add));
                    harpoon.push(
                        std::make_shared<EventMessage>("system",
                                                       nick + " has joined the channel",
                                                       MessageType::Status));
                }
                else if (cmd == "onlineRemove")
                {
                    const std::string& nick = root.get("nick", "").asString();
                    harpoon.push(
                        std::make_shared<EventMessage>("system",
                                                       nick + " has left the channel",
                                                       MessageType::Status));
                    harpoon.push(
                        std::make_shared<EventUserChanged>(nick, UserChangeType::Remove));
                }
                else if (cmd == "onlineSet")
                {
                    const auto& nicksArrayValue = root["nicks"];
                    if (nicksArrayValue.isArray())
                    {
                        std::vector<std::string> nicks(nicksArrayValue.size());
                        for (int i = 0; i < static_cast<int>(nicksArrayValue.size()); ++i)
                            nicks[i] = nicksArrayValue[i].asString();
                        harpoon.push(std::make_shared<EventUserList>(std::move(nicks)));
                    }
                }
            }
            catch(const std::exception& e)
            {
                harpoon.push(std::make_shared<EventMessage>("!!PARSE_ERROR!!", payload + ", " + e.what()));
            }
        });
    wss.set_open_handler(
        [this](auto hdl)
        {
            wssHandle = hdl;
            harpoon.push(std::make_shared<EventMessage>(
                "system",
                "connected to hack.chat...",
                MessageType::Status));
            Json::Value root;
            root["cmd"] = "join";
            root["channel"] = channel;
            root["nick"] = username + (password.empty() ? "" : "#" + password);

            WssErrorCode ec;
            wss.send(
                hdl,
                Json::writeString(Json::StreamWriterBuilder(), root),
                websocketpp::frame::opcode::text,
                ec);
            if (ec)
            {
                harpoon.push(std::make_shared<EventMessage>(
                    "system",
                    "failed to send message to hack.chat: " + ec.message(),
                    MessageType::Status));
            }
            wssPingThread = NJThread("WssPing",
                                     [this, hdl]
                                     {
                                         while (RUNNING && connected)
                                         {
                                             for (int i = 0; i < 60 && RUNNING; ++i)
                                             {
                                                 std::unique_lock lock(wssPingMutex);
                                                 wssPingCondition.wait_for(lock, std::chrono::seconds(1));
                                             }
                                             if (!RUNNING || !connected) break;
                                             WssErrorCode ec;
                                             wss.send(
                                                 hdl,
                                                 "{\"cmd\": \"ping\"}",
                                                 websocketpp::frame::opcode::text,
                                                 ec);
                                         }
                                     });
        });
    wss.set_fail_handler(
        [this](auto hdl)
        {
            harpoon.push(
                std::make_shared<EventMessage>(
                    "system",
                    "cxn error on hack.chat...",
                    MessageType::Status));
        });
    WssErrorCode ec;
    auto con = wss.get_connection(server, ec);
    if (ec)
    {
        connected = false;
        harpoon.push(std::make_shared<EventMessage>(
            "system",
            "failed to connect to hack.chat: " + ec.message(),
            MessageType::Status));
        return;
    }
    wss.connect(con);
    wssThread = NJThread("WssThread", [this]{wss.run();});
}
            
void Client::onHackConnected(EventHackConnected& event)
{
    harpoon.push(
        std::make_shared<EventMessage>(
            "system",
            "connected to hack.chat...",
            MessageType::Status));
}
void Client::onHackDisconnect(EventHackDisconnect& event)
{
    connected = false;
    harpoon.push(
        std::make_shared<EventMessage>(
            "system",
            "disconnecting from hack.chat...",
            MessageType::Status));
}
void Client::onHackDisconnected(EventHackDisconnected& event)
{
    if (connected)
    {
        harpoon.push(
            std::make_shared<EventMessage>(
                "system",
                "reconnecting to hack.chat...",
                MessageType::Status));
        queue.push(std::make_shared<EventHackConnect>(server, channel, username, password));
        connected = false;
    }
    else
    {
        harpoon.push(std::make_shared<EventMessage>(
            "system",
            "disconnected from hack.chat...",
            MessageType::Status));
    }
    std::lock_guard lock(wssPingMutex);
    wssPingCondition.notify_all();
}

}
