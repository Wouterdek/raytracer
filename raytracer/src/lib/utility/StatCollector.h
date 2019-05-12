#pragma once

#include <string>
#include <variant>
#include <vector>
#include <optional>


#define LOGSTAT(statCollector, entryName, value) if(statCollector != nullptr) {statCollector->log(__FUNCTION__, entryName, value);}

namespace Statistics
{
    using Value = std::variant<double, size_t, int, bool, std::string>;

    struct Entry
    {
        std::string source;
        std::string entryName;
        Value value;

        Entry(std::string source, std::string entryName, Statistics::Value value)
                : source(std::move(source)), entryName(std::move(entryName)), value(std::move(value))
        { }
    };

    class Collector
    {
    public:
        void log(const std::string &source, const std::string &entryName, Statistics::Value value)
        {
            entries.emplace_back(source, entryName, value);
        }

        const std::vector<Entry>& getEntries() const
        {
            return entries;
        }

        void clear()
        {
            entries.clear();
        }

        std::string getString() const;

    private:
        std::vector<Entry> entries;
    };
}