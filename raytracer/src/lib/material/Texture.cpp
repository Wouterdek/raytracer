#include "Texture.h"
#include <vector>
#include <cmath>
#include "io/PNG.h"

TextureUInt8 TextureUInt8::loadPNG(std::string path)
{
	//decode
    std::vector<unsigned char> imageData;
    unsigned width, height;
    read_from_png_file(imageData, width, height, path);
	
	return TextureUInt8(std::move(imageData), width, height);
}
