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
			out << static_cast<int>(pixel.getRed()*255) << " " << static_cast<int>(pixel.getGreen()*255) << " " << static_cast<int>(pixel.getBlue()*255) << " ";
		}
		out << std::endl;
	}

	out.close();
}
