#include "PNGWriter.h"
#include "lodepng.h"
#include "film/FrameBuffer.h"
#include <algorithm>

void write_to_png_file(const FrameBuffer& buffer, std::string filepath, double exposure, double gamma)
{
	std::vector<unsigned char> rgb(buffer.getHorizontalResolution() * buffer.getVerticalResolution() * 3);

	auto width = buffer.getHorizontalResolution();
	auto height = buffer.getVerticalResolution();
	auto exposureFactor = pow(2, exposure);
	for (auto y = 0; y < height; y++)
	{
		for (auto x = 0; x < width; x++)
		{
			auto offset = ((y * width) + x) * 3;
			auto pixel = (buffer.getPixel(x, y) * exposureFactor).pow(1/gamma);
			rgb[offset] = std::min(255, static_cast<int>(pixel.getRed() * 255));
			rgb[offset+1] = std::min(255, static_cast<int>(pixel.getGreen() * 255));
			rgb[offset+2] = std::min(255, static_cast<int>(pixel.getBlue() * 255));
		}
	}

	lodepng::encode(filepath, rgb, width, height, LodePNGColorType::LCT_RGB);
}
