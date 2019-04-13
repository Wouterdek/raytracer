#include "Tile.h"
#include <exception>
#include <ostream>
#include <algorithm>

Tile::Tile(int xStart, int yStart, int xEnd, int yEnd)
	: xStart(xStart), yStart(yStart), xEnd(xEnd), yEnd(yEnd)
{
	if(yStart > yEnd || xStart > xEnd)
	{
		throw std::invalid_argument("Invalid tile bounds");
	}
}

Tile::Tile()
	: xStart(0), yStart(0), xEnd(0), yEnd(0)
{}


int Tile::getWidth() const
{
	return this->xEnd - this->xStart;
}

int Tile::getHeight() const
{
	return this->yEnd - this->yStart;
}

std::vector<Tile> Tile::subdivide(int preferredWidth, int preferredHeight) const
{
	if (preferredWidth <= 0)
	{
		throw std::invalid_argument("the width of a tile must be larger than zero!");
	}
	else if (preferredHeight <= 0)
	{
		throw std::invalid_argument("the height of a tile must be larger than zero!");
	}

	std::vector<Tile> subtiles {};
	subtiles.reserve(preferredWidth*preferredHeight);

	for (int y = this->yStart; y < this->yEnd; y += preferredHeight) {
		for (int x = this->xStart; x < this->xEnd; x += preferredWidth) {
			int newXEnd = std::min(this->xEnd, x + preferredWidth);
			int newYEnd = std::min(this->yEnd, y + preferredHeight);
			subtiles.emplace_back(x, y, newXEnd, newYEnd);
		}
	}

	return subtiles;
}

int Tile::getXStart() const
{
	return this->xStart;
}

int Tile::getYStart() const
{
	return this->yStart;
}

int Tile::getXEnd() const
{
	return this->xEnd;
}

int Tile::getYEnd() const
{
	return this->yEnd;
}

std::ostream & operator<<(std::ostream& in, const Tile& tile)
{
	in << "[Tile]: " << tile.getXStart() << " " << tile.getYStart() << " " << tile.getXEnd() << " " << tile.getYEnd();
	return in;
}
