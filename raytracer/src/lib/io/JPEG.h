#pragma once
#include <string>
#include <vector>

void read_jpeg_file(std::vector<unsigned char>& out, unsigned int& width, unsigned int& height, const std::string& filename);
