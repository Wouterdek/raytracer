#pragma once
#include <string>

class FrameBuffer;
void write_to_png_file(const FrameBuffer& buffer, std::string filepath, double exposure, double gamma);
