#include <stdexcept>
#include "HDR.h"
#include "lib/stb_image.h"

void read_hdr_file(std::vector<float>& out, int& width, int& height, const std::string& filename)
{
    int bitDepth;
    int channelCount = 4;
    float* data = stbi_loadf(filename.c_str(), &width, &height, &bitDepth, channelCount);
    if(data == nullptr)
    {
        throw std::runtime_error("Failed to load HDR file");
    }

    size_t datasize = width * height * channelCount;
    out.resize(datasize);
    std::copy(data, data + datasize, out.begin());
}