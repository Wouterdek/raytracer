#include "Texture.h"
#include <vector>
#include "io/lodepng.h"

Texture::Texture()
{
}

RGB Texture::get(unsigned int x, unsigned int y) const
{
	auto offset = ((y * this->width) + x) * 4; //Left to right, 4 bytes per pixel
	auto r = this->image[offset];
	auto g = this->image[offset+1];
	auto b = this->image[offset+2];
	auto a = this->image[offset+3];
	return RGB((float)r/255.0f, (float)g / 255.0f, (float)b / 255.0f);
}

unsigned Texture::getWidth() const
{
	return width;
}

unsigned Texture::getHeight() const
{
	return height;
}

Texture Texture::load(std::string path)
{
	Texture tex;

	//decode
	unsigned error = lodepng::decode(tex.image, tex.width, tex.height, path);
	
	return tex;
}
