#pragma once

#include "scene/renderable/Scene.h"
#include "film/FrameBuffer.h"
#include "utility/ProgressMonitor.h"

struct RenderSettings
{
	int aaLevel = 4;
};

void render(const Scene& scene, FrameBuffer& buffer, const Tile& tile, const RenderSettings& renderSettings, ProgressMonitor progressMon, bool multithreaded = true);

void render(const Scene& scene, FrameBuffer& buffer, const RenderSettings& renderSettings, ProgressMonitor progressMon);
