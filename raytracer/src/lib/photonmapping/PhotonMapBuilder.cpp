#include "PhotonMapBuilder.h"
#include "PhotonMap.h"
#include "math/OrthonormalBasis.h"
#include "math/Sampler.h"
#include "math/Ray.h"
#include "Photon.h"
#include "KDTree.h"
#include "KDTreeBuilder.h"
#include <vector>
#include "PhotonTracer.h"

using size_type = std::vector<Photon>::size_type;

PhotonMap PhotonMapBuilder::buildPhotonMap(const Scene& scene, PhotonMapMode mode, size_t photonsPerAreaLight, size_t photonsPerPointLight, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);
    PhotonList photons{};

    if(mode != PhotonMapMode::none)
    {
        PhotonTracer tracer{};
        tracer.batchSize = 10000;
        tracer.photonsPerAreaLight = photonsPerAreaLight;
        tracer.photonsPerPointLight = photonsPerPointLight;
        tracer.mode = mode;
        tracer.tracePhotons(scene, photons, progressMon);
    }

    // Build KD-tree from photon list
    std::stringstream msg;
    msg << "Building photon KD-tree (" << photons.size() << " photons)";
    progress.startNewJob(msg.str(), 1);
    auto tree = KDTreeBuilder::build(photons);
    progress.signalTaskFinished();

    /*progress.startNewJob("Packing photon KD-tree", 1);
    tree.pack();
    progress.signalTaskFinished();*/

    return tree;
}

PhotonRayMap PhotonMapBuilder::buildPhotonRayMap(const Scene& scene, PhotonMapMode mode, size_t photonsPerAreaLight, size_t photonsPerPointLight, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);
    PhotonRayList photons{};

    if(mode != PhotonMapMode::none)
    {
        PhotonTracer tracer{};
        tracer.batchSize = 10000;
        tracer.photonsPerAreaLight = photonsPerAreaLight;
        tracer.photonsPerPointLight = photonsPerPointLight;
        tracer.mode = mode;
        tracer.tracePhotonRays(scene, photons, progressMon);
    }

    return photons;
}