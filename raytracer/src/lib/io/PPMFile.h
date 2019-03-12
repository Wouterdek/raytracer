#pragma once

#include "film/FrameBuffer.h"
#include <string>

void write_to_ppm(const FrameBuffer& buffer, std::string filepath);