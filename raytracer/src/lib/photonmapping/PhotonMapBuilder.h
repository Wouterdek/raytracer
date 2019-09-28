#pragma once

#include "scene/renderable/Scene.h"
#include "utility/ProgressMonitor.h"
#include "PhotonMap.h"

class PhotonMapBuilder {
public:
    static PhotonMap buildPhotonMap(const Scene& scene, PhotonMapMode mode, ProgressMonitor progressMon);
};
