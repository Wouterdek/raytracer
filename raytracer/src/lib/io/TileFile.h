#pragma once
#include <string>

class FrameBuffer;
class Tile;
void write_to_tile_file(const FrameBuffer& buffer, const Tile& tile, std::string filepath);
void load_from_tile_file(FrameBuffer& buffer, std::string filepath);
