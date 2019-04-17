#include "Texture.h"
#include <vector>
#include "io/lodepng.h"

Texture::Texture(std::vector<unsigned char> image, unsigned width, unsigned height)
    : image(std::move(image)), width(width), height(height)
{}

Texture Texture::loadPNG(std::string path)
{
	//decode
    std::vector<unsigned char> imageData;
    unsigned width, height;
	unsigned error = lodepng::decode(imageData, width, height, path);
	
	return Texture(std::move(imageData), width, height);
}
