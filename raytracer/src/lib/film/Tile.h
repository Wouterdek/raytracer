#pragma once
#include <vector>
#include <sstream>

class Tile
{
public:
	Tile(int xStart, int yStart, int xEnd, int yEnd);
	int getWidth() const;
	int getHeight() const;
	std::vector<Tile> subdivide(int preferredWidth, int preferredHeight) const;

	int getXStart() const;
	int getYStart() const;
	int getXEnd() const;
	int getYEnd() const;

private:
	int xStart;
	int yStart;
	int xEnd;
	int yEnd;
};

std::ostream& operator<<(std::ostream& in, const Tile& tile);
