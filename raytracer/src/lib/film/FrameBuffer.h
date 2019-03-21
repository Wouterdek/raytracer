#pragma once

#include <vector>
#include "RGB.h"
#include "Tile.h"

class FrameBuffer
{
public:
	FrameBuffer(int xResolution, int yResolution);

	std::vector<Tile> subdivide(int width, int height) const;

	const RGB& getPixel(int x, int y) const
	{
		return this->pixels[(y * horResolution) + x];
	}

	void setPixel(int x, int y, RGB value)
	{
		this->pixels[(y * horResolution) + x] = value;
	}

	int getHorizontalResolution() const
	{
		return this->horResolution;
	}

	int getVerticalResolution() const
	{
		return this->vertResolution;
	}

private:
	/*
	 * Two-dimensional array of pixels. The pixels are stored in row order. When
	 * iterating over the pixels, one should first iterate over the y
	 * coordinates, followed by the x coordinates for optimal performance.
	 */
	std::vector<RGB> pixels;

	/**
	 * The horizontal resolution of this frame buffer.
	 */
	int horResolution;

	/**
	 * The vertical resolution of this frame buffer.
	 */
	int vertResolution;
};
