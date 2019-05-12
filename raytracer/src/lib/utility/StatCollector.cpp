#include "StatCollector.h"
#include <sstream>
#include <iomanip>

//streaming variants: https://stackoverflow.com/a/47169101
template<class T>
struct streamer {
    const T& val;
};
template<class T> streamer(T) -> streamer<T>;

template<class T>
std::ostream& operator<<(std::ostream& os, streamer<T> s) {
    os << s.val;
    return os;
}

template<class... Ts>
std::ostream& operator<<(std::ostream& os, streamer<std::variant<Ts...>> sv) {
    std::visit([&os](const auto& v) { os << streamer{v}; }, sv.val);
    return os;
}


std::string Statistics::Collector::getString() const
{
    std::ostringstream ss{};
    for(const auto& entry : entries)
    {
        ss << std::left << std::setw(20) << std::setfill(' ') << entry.entryName + ":";
        ss << std::left << std::setw(20) << std::setfill(' ') << streamer{entry.value} << std::endl;;
    }

    return ss.str();
}
