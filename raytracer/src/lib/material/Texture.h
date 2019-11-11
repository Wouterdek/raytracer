#pragma once
#include "film/RGB.h"
#include <vector>

template<typename DataType>
class Texture
{
private:
	std::vector<DataType> image; //the raw pixels in RGBA order
	unsigned int width, height;
	double gamma = 1.0;
	float divisor;

public:
    Texture(std::vector<DataType> image, unsigned int width, unsigned int height, float divisor)
            : image(std::move(image)), width(width), height(height), divisor(divisor)
    {
        if(this->image.size() != width * height * 4) //RGBA = 4 channels
        {
            throw std::runtime_error("Invalid image buffer size. Must be of size width*height*4");
        }
    }

	RGB get(unsigned int x, unsigned int y) const
    {
        auto offset = ((y * this->width) + x) * 4; //Left to right, 4 components per pixel, 1 DataType per component
        auto r = image[offset];
        auto g = image[offset+1];
        auto b = image[offset+2];
        auto a = image[offset+3];
		auto color = RGB{ (float)r / divisor, (float)g / divisor, (float)b / divisor };
        return color.pow(this->gamma);
    }

    unsigned int getWidth() const
    {
        return width;
    }

	unsigned int getHeight() const
    {
        return height;
    }

    std::vector<DataType>& data()
    {
	    return image;
	}

	void setGammaFactor(double gamma) { this->gamma = gamma; }
    double getGammaFactor() { return this->gamma; }
};

class TextureUInt8 : public Texture<unsigned char>
{
public:
    TextureUInt8(const std::vector<unsigned char>& image, unsigned int width, unsigned int height)
     : Texture(image, width, height, 255.0f){}

    static TextureUInt8 loadPNG(std::string path);
};
