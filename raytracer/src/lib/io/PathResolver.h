#pragma once

#include <filesystem>

class PathResolver {
public:
    static PathResolver& get()
    {
        static PathResolver instance;
        return instance;
    }

    PathResolver(PathResolver const &) = delete;
    void operator=(PathResolver const &) = delete;

    std::filesystem::path resolve(const std::string& path) const;
    void setWorkDir(const std::string& path);

private:
    PathResolver() = default;
    ~PathResolver() = default;

    std::string workdir;
};
