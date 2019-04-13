#include "TileFile.h"
#include "film/FrameBuffer.h"
#include "film/Tile.h"
#include <fstream>

std::string MAGIC = "TILE";

void write_to_tile_file(const FrameBuffer& buffer, const Tile& tile, std::string filepath)
{
	std::ofstream out;
	out.open(filepath, std::ios::out | std::ios::trunc | std::ios::binary);
	out.imbue(std::locale::classic());
	out.write(MAGIC.c_str(), MAGIC.length());

	int buf[2] = { buffer.getHorizontalResolution(), buffer.getVerticalResolution() };
	out.write(reinterpret_cast<const char*>(buf), sizeof(buf));
	out.write(reinterpret_cast<const char*>(&tile), sizeof(tile));

	for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
		for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
			auto color = buffer.getPixel(x, y);
			out.write(reinterpret_cast<const char*>(&color), sizeof(color));
		}
	}

	out.close();
}

void load_from_tile_file(FrameBuffer& buffer, std::string filepath)
{
	std::ifstream in;
	in.open(filepath, std::ios::in | std::ios::binary);
	in.imbue(std::locale::classic());
	
	char magic[4];
	in.read(magic, sizeof(magic));
	if(std::string(magic, sizeof(magic)) != MAGIC)
	{
		throw std::runtime_error("Bad file");
	}

	int horRes, verRes;
	in.read(reinterpret_cast<char*>(&horRes), sizeof(horRes));
	in.read(reinterpret_cast<char*>(&verRes), sizeof(verRes));

	if (buffer.getHorizontalResolution() != horRes || buffer.getVerticalResolution() != verRes)
	{
		throw std::runtime_error("Resolution mismatch");
	}

	Tile tile;
	in.read(reinterpret_cast<char*>(&tile), sizeof(tile));

	if (tile.getXStart() > tile.getXEnd() || tile.getYStart() > tile.getYEnd() ||
		tile.getXStart() < 0 || tile.getXEnd() < 0 || tile.getYStart() < 0 || tile.getYEnd() < 0 ||
		tile.getXEnd() > buffer.getHorizontalResolution() || tile.getYEnd() > buffer.getVerticalResolution())
	{
		throw std::runtime_error("Invalid tile geometry specification");
	}

	for (int y = tile.getYStart(); y < tile.getYEnd(); ++y) {
		for (int x = tile.getXStart(); x < tile.getXEnd(); ++x) {
			RGB color;
			in.read(reinterpret_cast<char*>(&color), sizeof(color));
			buffer.setPixel(x, y, color);
		}
	}

	in.close();
}