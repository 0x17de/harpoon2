#include "NCurses.hpp"
#include <iostream>
#include <sstream>
#include <map>
#include <algorithm>
#include <iterator>
#include <string_view>
#include <boost/date_time.hpp>
#include <poll.h>
#include <ncurses.h>
#include <utf8.h>
#include "Events.hpp"
#include "HackChatEvents.hpp"
#include "enums/MessageType.hpp"
#include "enums/UserChangeType.hpp"
#include "globals.hpp"

#define COLOR_GRAY37 59
#define COLOR_DARKGREEN 22
#define COLOR_DARKORANGE3 166
#define PAIR_BG 1
#define PAIR_INPUTLINE 2
#define PAIR_TRIP 3
#define PAIR_MOD 4
#define PAIR_STATUS 5
#define PAIR_MENTION 6

class BacklogMessage
{
public:
    BacklogMessage(const EventMessage& event);
    std::vector<std::string>& getMessageWithBreaks(size_t messageWidth);
    size_t getMessageLines(size_t messageWidth);
    const EventMessage& getEvent() const;
    size_t getPrefixLength();

private:
    /// constructs a string which already considers linebreaks and can be printed in one step
    void computeMessageWithBreaks(size_t messageWidth);

    EventMessage event;
    size_t calculatedPrefixLength;
    size_t calculatedMessageWidth;
    std::vector<std::string> messageWithBreaks;
};

template<class Map, class K, class V>
static auto getMapValue(const Map& map, const K& key, V&& otherwise)
{
    auto it = map.find(key);
    if (it != map.end()) return it->second;
    return otherwise;
}

template<class Map, class K, class V>
static bool compareMapValue(const Map& map, const K& key, const V& value)
{
    auto it = map.find(key);
    if (it == map.end()) return false;
    return it->second == value;
}


BacklogMessage::BacklogMessage(const EventMessage& event)
    : event(event)
    , calculatedPrefixLength(0)
    , calculatedMessageWidth(0)
    , messageWithBreaks()
{
}
const EventMessage& BacklogMessage::getEvent() const
{
    return event;
}
std::vector<std::string>& BacklogMessage::getMessageWithBreaks(size_t maxMessageWidth)
{
    computeMessageWithBreaks(maxMessageWidth);
    return messageWithBreaks;
}
size_t BacklogMessage::getMessageLines(size_t maxMessageWidth)
{
    computeMessageWithBreaks(maxMessageWidth);
    return messageWithBreaks.size();
}
size_t BacklogMessage::getPrefixLength()
{
    if (calculatedPrefixLength != 0) return calculatedPrefixLength;

    const bool isMod = compareMapValue(event.extra, "mod", "1");
    const bool isMe = event.type == MessageType::Me;
    const bool isWhisper = event.type == MessageType::Whisper;
    const bool isStatus = event.type == MessageType::Status;
    const std::string& trip = getMapValue(event.extra,
                                          "trip",
                                          std::string());
    calculatedPrefixLength = (trip.empty() ? 0 : trip.size()+1)
                             + ((isMe || isWhisper) ? 0 : event.sender.size() + 3);
    return calculatedPrefixLength;
}

void BacklogMessage::computeMessageWithBreaks(size_t maxMessageWidth)
{
    if (calculatedMessageWidth == maxMessageWidth) return;
    const size_t firstLinePrefixLenght = getPrefixLength();

    messageWithBreaks.clear();

    // only do updates if there is enough space to display anything
    if (maxMessageWidth > firstLinePrefixLenght)
    {
        std::wstringstream wmessageWithBreaksStream;
        size_t index = 0;
        size_t lastCopyEnd = 0;
        size_t offsetCount = 0;

        std::string out;
        std::wstring wout;

        std::wstring wmessage;
        utf8::utf8to32(event.message.begin(), event.message.end(),
                       std::back_inserter(wmessage));
        for (int j = 0; j < wmessage.size(); ++j)
        {
            ++index;
            ++offsetCount;
            if (event.message[j] == '\n')
            {
                wout = std::wstring(&wmessage[lastCopyEnd], offsetCount-1);
                out.clear();
                utf8::utf32to8(wout.begin(), wout.end(), std::back_inserter(out));
                messageWithBreaks.push_back(std::move(out));
                lastCopyEnd = index;
                offsetCount = 0;
                if (event.message[j+1] == '\0') { continue; }
            }
            else if (offsetCount >= maxMessageWidth - (messageWithBreaks.size() ? 0 : firstLinePrefixLenght))
            {
                wout = std::wstring(&wmessage[lastCopyEnd], offsetCount);
                out.clear();
                utf8::utf32to8(wout.begin(), wout.end(), std::back_inserter(out));
                messageWithBreaks.push_back(std::move(out));
                lastCopyEnd = index;
                offsetCount = 0;
                if (event.message[j+1] == '\n') { ++lastCopyEnd; ++j; }
                if (event.message[j+1] == '\0') { continue; }
            }
        }
        wout = std::wstring(&wmessage[lastCopyEnd], offsetCount);
        out.clear();
        utf8::utf32to8(wout.begin(), wout.end(), std::back_inserter(out));
        messageWithBreaks.push_back(std::move(out));
    }
    calculatedMessageWidth = maxMessageWidth;
}

NCurses::NCurses(EventQueue& queue, EventQueue& hackChatQueue)
    : queue(queue)
    , hackChatQueue(hackChatQueue)
{
    setlocale(LC_ALL, ""); 
    initscr();
    eventHandlerThread = NJThread(
        "ncursesEventHandler",
        [this]
        {
            while (RUNNING)
            {
                if (const auto& optMessage = this->queue.pop())
                    (*optMessage)->handle(this);
            }
        });
    t = NJThread(
        "NCurses",
        [this]
        {
            int newdy, newdx, dy, dx, usersw_dx=30; // dimensions
            bool    resize = false,
              redrawborder = false,
               redrawinput = false,
                redrawchat = false;
            redraw = true;
            redrawusers = false;
            scrollOffset = 0;

            start_color();
            use_default_colors();
            assume_default_colors(-1, -1);
#ifdef USE_DEBUGLOG
            std::ofstream("ncurses.log", std::ios_base::app) << "can change colors? " << std::boolalpha << can_change_color() << ", NCOLORS=" << COLORS << '\n';
#endif
            init_pair(PAIR_BG, COLOR_WHITE, COLOR_BLACK);
            init_pair(PAIR_INPUTLINE, COLOR_YELLOW, COLOR_BLUE);
            init_pair(PAIR_TRIP, COLOR_GRAY37, COLOR_BLACK);
            init_pair(PAIR_MOD, COLOR_WHITE, COLOR_DARKGREEN);
            init_pair(PAIR_STATUS, COLOR_YELLOW, COLOR_BLACK);
            init_pair(PAIR_MENTION, COLOR_WHITE, COLOR_DARKORANGE3);

            clear();
            noecho();
            cbreak();
            nonl();
            curs_set(0);
            getmaxyx(stdscr, dy, dx);
            newdx = dx;
            newdy = dy;

            WINDOW *w, *usersw, *inputw;

            while (RUNNING)
            {
                int k;
                static int time = 0;

                if (redraw)
                {
                    if (newdy != dy || newdx != dx)
                    {
                        endwin();
                        refresh();
                        delwin(inputw);
                        delwin(chatw);
                        delwin(usersw);
                        delwin(w);
                        inputw = 0;
                        chatw = 0;
                        usersw = 0;
                        w = 0;

                        resize = true;
                        dx = newdx;
                        dy = newdy;
                        touchwin(stdscr);
                        wnoutrefresh(stdscr);
#ifdef USE_DEBUGLOG
                        int ty, tx;
                        getmaxyx(stdscr, ty, tx);
                        std::ofstream("ncurses.log", std::ios_base::app) << "Resized stdscr to " << tx << "x" << ty << "\n";
#endif
                    }
                    redrawborder = true;
                    redrawchat = true;
                    redrawusers = true;
                    redrawinput = true;
                    redraw = false;
                }
                if (redrawborder)
                {
                    if (!w)
                    {
                        w = newwin(dy, dx, 0, 0);
                        wbkgd(w, COLOR_PAIR(PAIR_BG));
                    }
                    werase(w);
                    if (resize)
                    {
                        touchwin(w);
                        wnoutrefresh(w);
                        wresize(w, dy, dx);
#ifdef USE_DEBUGLOG
                        int ty, tx;
                        getmaxyx(w, ty, tx);
                        std::ofstream("ncurses.log", std::ios_base::app) << "Resized w to " << tx << "x" << ty << "\n";
#endif
                    }
                    wborder(w, 0, 0, 0, 0, 0, ACS_TTEE, 0, ACS_BTEE);
                    mvwprintw(w, dy-1, 1, "F10-Quit");
                    wrefresh(w);
                    redrawborder = false;
                }
                if (redrawchat)
                {
                    if (!chatw)
                    {
                        chatw = subwin(w, dy-3, dx-2-usersw_dx, 1, 1);
                    }
                    werase(chatw);
                    if (resize)
                    {
                        touchwin(chatw);
                        wnoutrefresh(chatw);
                        wresize(chatw, dy-3, dx-2-usersw_dx);
#ifdef USE_DEBUGLOG
                        int ty, tx;
                        getmaxyx(chatw, ty, tx);
                        std::ofstream("ncurses.log", std::ios_base::app) << "Resized chat to " << tx << "x" << ty << "\n";
#endif
                    }
                    {
                        int i = -scrollOffset;
                        int iMax = dy-3;
                        std::lock_guard lock(backlogMutex);
                        for (auto& backlogMessage : backlog)
                        {
                            if (i >= iMax) break;
                            const bool isMod = compareMapValue(backlogMessage.getEvent().extra, "mod", "1");
                            const bool isMe = backlogMessage.getEvent().type == MessageType::Me;
                            const bool isWhisper = backlogMessage.getEvent().type == MessageType::Whisper;
                            const bool isStatus = backlogMessage.getEvent().type == MessageType::Status;
                            const std::string& trip = getMapValue(backlogMessage.getEvent().extra,
                                                                  "trip",
                                                                  std::string());
                            size_t maxMessageWidth = getmaxx(chatw)-11;
                            const std::vector<std::string>& message =
                                    backlogMessage.getMessageWithBreaks(maxMessageWidth);
                            // shift chat N lines up
                            i += backlogMessage.getMessageLines(maxMessageWidth);

                            if (i > 0 && i < dy-2)
                            {
                                std::stringstream ss;
                                ss.imbue(
                                    std::locale(
                                        std::locale::classic(),
                                        new boost::posix_time::time_facet("%H:%M:%S")));
                                ss << backlogMessage.getEvent().time;
                                const std::string& timeStr = ss.str();
                                mvwaddnstr(chatw, dy-3-i, 0, timeStr.c_str(), timeStr.size());
                                waddstr(chatw, " | ");
                                if (isStatus) wattron(chatw, COLOR_PAIR(PAIR_STATUS));
                                if (isMe || isWhisper) wattron(chatw, A_ITALIC);
                                if (!isMe && !isWhisper) waddstr(chatw, isStatus?"[":"<");
                                if (!trip.empty())
                                {
                                    wattron(chatw, COLOR_PAIR(PAIR_TRIP));
                                    waddnstr(chatw, trip.c_str(), trip.size());
                                    wattroff(chatw, COLOR_PAIR(PAIR_TRIP));
                                    waddch(chatw, ' ');
                                }
                                if (isWhisper || isMe) wattron(chatw, COLOR_PAIR(PAIR_STATUS));
                                if (!isMe && !isWhisper)
                                {
                                    if (isMod) wattron(chatw, COLOR_PAIR(PAIR_MOD));
                                    const auto& sender = backlogMessage.getEvent().sender;
                                    waddnstr(chatw, sender.c_str(), sender.size());
                                    if (isMod) wattroff(chatw, COLOR_PAIR(PAIR_MOD));
                                }
                                if (!isMe && !isWhisper) waddstr(chatw, isStatus?"]":">");
                                if (!isMe && !isWhisper) waddch(chatw, ' ');
                            }
                            if (isWhisper || isMe) wattron(chatw, COLOR_PAIR(PAIR_STATUS));
                            for (int j = 0; j < message.size(); ++j)
                            {
                                if (i>j && i-j < dy-2)
                                {
                                    mvwaddnstr(chatw,
                                               dy-3-i+j, (j == 0 ? 11+backlogMessage.getPrefixLength() : 11),
                                               message[j].c_str(), message[j].size());
                                }
                            }
                            if (isMe || isWhisper) wattroff(chatw, A_ITALIC);
                            if (isStatus || isMe || isWhisper) wattroff(chatw, COLOR_PAIR(PAIR_STATUS));
                        }
                    }
                    wrefresh(chatw);
                    redrawchat = false;
                }
                if (redrawusers)
                {
                    if (!usersw)
                    {
                        usersw = subwin(w, dy, usersw_dx, 0, dx-usersw_dx);
                    }
                    werase(usersw);
                    if (resize)
                    {
                        touchwin(usersw);
                        wnoutrefresh(usersw);
                        wresize(usersw, getmaxy(w), usersw_dx);
                        mvwin(usersw, 0, getmaxx(w)-usersw_dx);
                    }
                    wborder(usersw, 0, 0, 0, 0, ACS_TTEE, 0, ACS_BTEE, 0);
                    {
                        int i = 0;
                        std::lock_guard lock(usersMutex);
                        for (const auto& user : users)
                        {
                            if (i >= dy-1) break;
                            mvwaddnstr(usersw, ++i, 1, user.c_str(), user.size());
                        }
                    }
                    wrefresh(usersw);
                    redrawusers = false;
                }
                if (redrawinput)
                {
                    if (!inputw)
                    {
                        inputw = subwin(w, 1, dx-2-usersw_dx, dy-2, 1);
                        keypad(inputw, 1);
                        wtimeout(inputw, 1000);
                        wbkgd(inputw, COLOR_PAIR(PAIR_INPUTLINE));
                    }
                    werase(inputw);
                    mvwprintw(inputw, 0, 0, "%s", buffer.c_str());
                    wrefresh(inputw);
                    redrawinput = false;
                }
                k = wgetch(inputw);
                if (k != ERR)
                {
                    lastk = k;
                    if (k == KEY_RESIZE) // terminal was resized
                    {
                        getmaxyx(stdscr, newdy, newdx);
                        redraw = true;
                    }
                    else if (k == KEY_UP)
                    {
                        scrollOffset += 1;
                        redrawchat = true;
                    }
                    else if (k == KEY_DOWN)
                    {
                        if (scrollOffset > 0) --scrollOffset;
                        else scrollOffset = 0;
                        redrawchat = true;
                    }
                    else if (k == KEY_END)
                    {
                        scrollOffset = 0;
                        redrawchat = true;
                    }
                    else if (k == KEY_PPAGE)
                    {
                        scrollOffset += dy-3;
                        redrawchat = true;
                    }
                    else if (k == KEY_NPAGE)
                    {
                        if (dy-3 > scrollOffset) scrollOffset = 0;
                        else scrollOffset -= dy-3;
                        redrawchat = true;
                    }
                    else if (k == KEY_F(10))
                    {
                        RUNNING = false;
                        break;
                    }
                    else if (k >= ' ' && k <= '~')
                    {
                        buffer += k;
                        redrawinput = true;
                    }
                    else if (k == KEY_BACKSPACE || k == 8 || k == 127)
                    {
                        if (!buffer.empty()) buffer.resize(buffer.size()-1);
                        redrawinput = true;
                    }
                    else if (k == '\r' || k == '\n')
                    {
                        this->queue.push(std::make_shared<EventInput>(buffer));
                        buffer = "";
                        redrawinput = true;
                    }
                }
            }
        });
}

NCurses::~NCurses()
{
    RUNNING = false;
    t.join();
    clear();
    endwin();
}

void NCurses::onInput(EventInput& event)
{
    hackChatQueue.push(std::make_shared<EventHackSendMessage>(event.message));
}

void NCurses::onUserList(EventUserList& event)
{
    std::lock_guard lock(usersMutex);
    users = event.users;
    redrawusers = true;
}
void NCurses::onUserChanged(EventUserChanged& event)
{
    std::lock_guard lock(usersMutex);
    switch (event.changeType)
    {
        case UserChangeType::Add:
            users.push_back(event.user);
            break;
        case UserChangeType::Remove:
            users.erase(std::find(users.begin(),
                                  users.end(),
                                  event.user));
            break;
    }
    redrawusers = true;
}
void NCurses::onMessage(EventMessage& event)
{
    addMessage(event);
}

void NCurses::addMessage(const EventMessage& message)
{
    std::lock_guard lock(backlogMutex);
    backlog.push_front(BacklogMessage(message));
    BacklogMessage& msg = backlog.front();
    while (backlog.size() > 800) backlog.pop_back();
    redraw = true;
    if (scrollOffset > 0) scrollOffset += msg.getMessageLines(getmaxx(chatw)-11);
}
