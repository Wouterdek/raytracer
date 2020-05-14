#pragma once

#include "KDTree.h"
#include "Photon.h"
#include "scene/renderable/Scene.h"
#include "utility/ProgressMonitor.h"

class PhotonTracer
{
public:
    using size_type = std::vector<Photon>::size_type;

    size_type photonsPerPointLight = 1E6;
    size_type photonsPerAreaLight = 1E6;
    size_type batchSize = 1000;
    PhotonMapMode mode;

    void tracePhotons(const Scene& scene, PhotonList& photons, ProgressMonitor progress);
    void tracePhotonRays(const Scene& scene, PhotonRayList& photons, ProgressMonitor progress);
};