#pragma once
#include "film/RGB.h"
#include <vector>

template<typename DataType>
class Texture
{
private:
	std::vector<unsigned char> image; //the raw pixels in RGBA order
	unsigned int width, height;
	double gamma = 1.0;
	float maxValue;

public:
    Texture(std::vector<unsigned char> image, unsigned width, unsigned height, float maxValue)
            : image(std::move(image)), width(width), height(height), maxValue(maxValue)
    {
        if(this->image.size() != width * height * 4 * sizeof(DataType)) //RGBA = 4 channels
        {
            throw std::runtime_error("Invalid image buffer size. Must be of size width*height*4*sizeof(DataType)");
        }
    }

	RGB get(unsigned int x, unsigned int y) const
    {
        auto values = reinterpret_cast<const DataType*>(&image[0]);
        auto offset = ((y * this->width) + x) * 4; //Left to right, 4 components per pixel, 1 DataType per component
        auto r = values[offset];
        auto g = values[offset+1];
        auto b = values[offset+2];
        auto a = values[offset+3];
		auto color = RGB{ (float)r / maxValue, (float)g / maxValue, (float)b / maxValue };
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

    std::vector<unsigned char>& data()
    {
	    return image;
	}

	void setGammaFactor(double gamma) { this->gamma = gamma; }
    double getGammaFactor() { return this->gamma; }
};

class TextureUInt8 : public Texture<unsigned char>
{
public:
    TextureUInt8(const std::vector<unsigned char>& image, unsigned width, unsigned height)
     : Texture(image, width, height, 255.0f){}

    static TextureUInt8 loadPNG(std::string path);
};
