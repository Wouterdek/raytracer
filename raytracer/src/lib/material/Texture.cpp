#include "Texture.h"
#include <vector>
#include <cmath>
#include "io/PNG.h"

Texture::Texture(std::vector<unsigned char> image, unsigned width, unsigned height)
    : image(std::move(image)), width(width), height(height)
{
    if(this->image.size() != width * height * 4) //RGBA = 4 channels
    {
        throw std::runtime_error("Invalid image buffer size. Must be of size width*height*4");
    }
}

Texture Texture::loadPNG(std::string path)
{
	//decode
    std::vector<unsigned char> imageData;
    unsigned width, height;
    read_from_png_file(imageData, width, height, path);
	
	return Texture(std::move(imageData), width, height);
}
