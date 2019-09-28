#include "PhotonMapBuilder.h"
#include "PhotonMap.h"
#include "math/OrthonormalBasis.h"
#include "math/UniformSampler.h"
#include "math/Ray.h"
#include "Photon.h"
#include "KDTree.h"
#include "KDTreeBuilder.h"
#include <vector>
#include <tbb/tbb.h>
#include "PhotonTracer.h"

using size_type = std::vector<Photon>::size_type;

PhotonMap PhotonMapBuilder::buildPhotonMap(const Scene& scene, PhotonMapMode mode, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);
    tbb::concurrent_vector<Photon> photons{};

    if(mode != PhotonMapMode::none)
    {
        tbb::task_list tasks{};
        size_type taskCount = 0;
        PhotonTracer tracer{};
        tracer.batchSize = 10000;
        tracer.photonsPerAreaLight = 1E9;
        tracer.photonsPerPointLight = 1E8;
        tracer.mode = mode;
        tracer.createPhotonTracingTasks(scene, tasks, taskCount, photons, progress);

        progress.startNewJob("Tracing photons", taskCount);
        tbb::task::spawn_root_and_wait(tasks);
    }

    //return photons;
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
