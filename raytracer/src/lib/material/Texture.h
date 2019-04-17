#pragma once
#include "film/RGB.h"
#include <vector>

class Texture
{
private:
	std::vector<unsigned char> image; //the raw pixels in RGBA order
	unsigned width, height;

public:
    Texture(std::vector<unsigned char> image, unsigned width, unsigned height);
	static Texture loadPNG(std::string path);

	RGB get(unsigned int x, unsigned int y) const
    {
        auto offset = ((y * this->width) + x) * 4; //Left to right, 4 bytes per pixel
        auto r = this->image[offset];
        auto g = this->image[offset+1];
        auto b = this->image[offset+2];
        auto a = this->image[offset+3];
        return RGB((float)r/255.0f, (float)g / 255.0f, (float)b / 255.0f);
    }

    unsigned int getWidth() const
    {
        return width;
    }

	unsigned int getHeight() const
    {
        return height;
    }

    std::vector<unsigned char>& data()
    {
	    return image;
	}
};


