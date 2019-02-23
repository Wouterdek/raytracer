#pragma once

#include <vector>
#include "RGB.h"
#include "Tile.h"

class FrameBuffer
{
public:
	FrameBuffer(int xResolution, int yResolution);

	const RGB& getPixel(int x, int y) const;
	void setPixel(int x, int y, RGB value);
	std::vector<Tile> subdivide(int width, int height) const;

	int getHorizontalResolution() const;
	int getVerticalResolution() const;

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