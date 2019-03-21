#pragma once

#include "scene/renderable/Scene.h"
#include "film/FrameBuffer.h"

struct RenderSettings
{
	int aaLevel = 4;
};

void render(const Scene& scene, FrameBuffer& buffer, const Tile& tile, const RenderSettings& renderSettings, bool multithreaded = true);

void render(const Scene& scene, FrameBuffer& buffer, const RenderSettings& renderSettings);
