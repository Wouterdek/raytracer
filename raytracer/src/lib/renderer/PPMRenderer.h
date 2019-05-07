#pragma once

#include "Renderer.h"

// Progressive Photon Mapping Renderer
class PPMRenderer : public Renderer
{
public:
    void render(const Scene &scene, FrameBuffer &buffer, const Tile &tile, const RenderSettings &renderSettings, ProgressMonitor progressMon, bool multithreaded) override;
};
