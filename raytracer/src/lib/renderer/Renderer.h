#pragma once

#include "scene/renderable/Scene.h"
#include "film/FrameBuffer.h"
#include "utility/ProgressMonitor.h"
#include "film/Tile.h"

struct RenderSettings
{
	int geometryAAModifier = 4;
    int materialAAModifier = 32;
};

class Renderer
{
public:
    virtual void render(const Scene& scene, FrameBuffer& buffer, std::shared_ptr<FrameBuffer>& perfBuffer, const Tile& tile, const RenderSettings& renderSettings, ProgressMonitor progressMon, bool multithreaded);
    virtual void render(const Scene& scene, FrameBuffer& buffer, std::shared_ptr<FrameBuffer>& perfBuffer, const RenderSettings& renderSettings, ProgressMonitor progressMon);

protected:
    const ICamera& findCamera(const Scene& scene);

    std::vector<Tile> subdivideTilePerCores(const Tile& tile, int taskMultiplier = 3 /* tasks per cpu, to compensate for slow vs fast tasks */);
};
