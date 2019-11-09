#pragma once
#include <memory>
#include <variant>

template<class... Args>
struct EventList
{
    using type = std::variant<std::shared_ptr<Args>...>;
};
template<class... Args>
using EventList_t = typename EventList<Args...>::type;
