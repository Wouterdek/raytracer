#pragma once
#include "film/RGB.h"
#include <vector>

class Texture
{
private:
	std::vector<unsigned char> image; //the raw pixels in RGBA order
	unsigned width, height;
	Texture();

public:
	static Texture load(std::string path);

	RGB get(unsigned int x, unsigned int y) const;
	unsigned int getWidth() const;
	unsigned int getHeight() const;
};
