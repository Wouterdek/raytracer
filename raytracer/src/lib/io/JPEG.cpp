#include <stdexcept>
#include "JPEG.h"
#include "lib/stb_image.h"

void read_jpeg_file(std::vector<unsigned char> &out, unsigned int &width, unsigned int &height, const std::string &filename)
{
    int x, y, n;
    int desired_channels = 4; //RGBA
    unsigned char *data = stbi_load(filename.c_str(), &x, &y, &n, desired_channels);
    if(data == nullptr)
    {
        throw std::runtime_error("Failed to load image");
    }

    width = x;
    height = y;
    size_t byteCount = width * height * desired_channels;
    out.resize(byteCount);
    std::copy(data, data + byteCount, out.begin());
    stbi_image_free(data);
}
