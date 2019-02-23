#include "PPMFile.h"
#include <fstream>

void write_to_ppm(const FrameBuffer& buffer, std::string filepath)
{
	std::ofstream out;
	out.open(filepath, std::ios::out | std::ios::trunc);
	out << "P3" << std::endl;
	out << buffer.getHorizontalResolution() << " " << buffer.getVerticalResolution() << std::endl;
	out << 255 << std::endl;

	for(int y = 0; y < buffer.getVerticalResolution(); y++)
	{
		for (int x = 0; x < buffer.getHorizontalResolution(); x++)
		{
			auto& pixel = buffer.getPixel(x, y);
			out << (pixel.getRed()*255) << " " << (pixel.getGreen()*255) << " " << (pixel.getBlue()*255) << " ";
		}
		out << std::endl;
	}

	out.close();
}
