#pragma once
#include <string>
#include <vector>

class FrameBuffer;
void write_to_png_file(const FrameBuffer& buffer, std::string filepath, double exposure, double gamma);
void read_from_png_file(std::vector<unsigned char>& out, unsigned int& width, unsigned int& height, const std::string& filename);
