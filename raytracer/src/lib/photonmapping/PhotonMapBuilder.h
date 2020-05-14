#pragma once

#include "scene/renderable/Scene.h"
#include "utility/ProgressMonitor.h"
#include "PhotonMap.h"

class PhotonMapBuilder {
public:
    static PhotonMap buildPhotonMap(const Scene& scene, PhotonMapMode mode, size_t photonsPerAreaLight, size_t photonsPerPointLight, ProgressMonitor progressMon);
    static PhotonRayMap buildPhotonRayMap(const Scene& scene, PhotonMapMode mode, size_t photonsPerAreaLight, size_t photonsPerPointLight, ProgressMonitor progressMon);
};
