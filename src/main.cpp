#include <iostream>
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#include <variant>
#include <map>
#include <sstream>
#include "Queue.hpp"
#include "JThread.hpp"
#include "SimpleSignalHandler.hpp"
#include "HackChatClient.hpp"
#include "NCurses.hpp"
#include "HackChatEvents.hpp"

namespace po = boost::program_options;

bool RUNNING = true;

int main(int argc, char* argv[])
{
    std::string username, password, channel;
    {
        po::options_description desc("Options");
        po::variables_map vm;

        try
        {
            desc.add_options()
                    ("help", "Show this help")
                    ("username", po::value<std::string>()->required(), "The username")
                    ("password", po::value<std::string>(), "The password")
                    ("channel", po::value<std::string>()->default_value("programming"), "The channel name without #");

            po::store(po::parse_command_line(argc, argv, desc), vm);

            if (vm.count("help"))
            {
                std::cout << desc << std::endl;
                return 0;
            }

            po::notify(vm);

            username = vm["username"].as<std::string>();
            password = vm.count("password") ? vm["password"].as<std::string>() : std::string();
            channel = vm["channel"].as<std::string>();
        }
        catch(po::error& e)
        {
            std::cerr << "ArgumentError: " << e.what() << "\n\n"
                      << desc << std::endl;
            return 1;
        }
    }

    SimpleSignalHandler simpleSignalHandler;

    EventQueue ncursesQueue;
    hackchat::Client hackChatClient(ncursesQueue);

    NCurses ncurses(ncursesQueue, hackChatClient.queue);

    hackChatClient.queue.push(EventHackConnect("wss://hack.chat/chat-ws", channel, username, password));

    simpleSignalHandler.handle();
    return 0;
}
