#pragma once
#include <string>

class FrameBuffer;
void write_to_exr_file(const FrameBuffer& buffer, std::string filepath, bool full_floats);
