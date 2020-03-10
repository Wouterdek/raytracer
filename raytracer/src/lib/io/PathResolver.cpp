#include "PathResolver.h"
#include <iostream>
std::string replaceAll(const std::string& in, const std::string& toReplace, const std::string& replacement)
{
    std::string str = in;

    size_t pos;
    while((pos = str.find(toReplace)) != std::string::npos)
    {
        str.replace(pos, toReplace.size(), replacement);
    }
    return str;
}

std::filesystem::path PathResolver::resolve(const std::string& path) const
{
    auto resolved = replaceAll(path, "#WORKDIR#", workdir);
    return std::filesystem::path(resolved);
}

void PathResolver::setWorkDir(const std::string &path) {
    this->workdir = path;
}
