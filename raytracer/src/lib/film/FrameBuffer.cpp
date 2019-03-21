#include "FrameBuffer.h"
#include <exception>

FrameBuffer::FrameBuffer(int horResolution, int vertResolution)
	: horResolution(horResolution), vertResolution(vertResolution)
{
	if(horResolution <= 0)
	{
		throw std::invalid_argument("Horizontal resolution must be larger than zero");
	}
	else if(vertResolution <= 0)
	{
		throw std::invalid_argument("Vertical resolution must be larger than zero");
	}

	pixels = std::vector<RGB>(horResolution * vertResolution);
}

std::vector<Tile> FrameBuffer::subdivide(int width, int height) const
{
	return Tile(0, 0, horResolution, vertResolution).subdivide(width, height);
}
