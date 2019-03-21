#include "PNGWriter.h"
#include "lodepng.h"
#include "film/FrameBuffer.h"

void write_to_png_file(const FrameBuffer& buffer, std::string filepath)
{
	std::vector<unsigned char> rgb(buffer.getHorizontalResolution() * buffer.getVerticalResolution() * 3);

	auto width = buffer.getHorizontalResolution();
	auto height = buffer.getVerticalResolution();
	for (auto y = 0; y < height; y++)
	{
		for (auto x = 0; x < width; x++)
		{
			auto offset = ((y * width) + x) * 3;
			auto pixel = buffer.getPixel(x, y);
			rgb[offset] = static_cast<int>(pixel.getRed() * 255);
			rgb[offset+1] = static_cast<int>(pixel.getGreen() * 255);
			rgb[offset+2] = static_cast<int>(pixel.getBlue() * 255);
		}
	}

	lodepng::encode(filepath, rgb, width, height, LodePNGColorType::LCT_RGB);
}
