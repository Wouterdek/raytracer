#include "PhotonMapBuilder.h"
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

PhotonMap PhotonMapBuilder::buildPhotonMap(const Scene& scene, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);
    tbb::concurrent_vector<Photon> photons{};

    {
        tbb::task_list tasks{};
        size_type taskCount = 0;
        PhotonTracer tracer{};
        tracer.batchSize = 1000;
        tracer.photonsPerAreaLight = 1E6;
        tracer.photonsPerPointLight = 1E6;
        tracer.createPhotonTracingTasks(scene, tasks, taskCount, photons, progress);

        progress.startNewJob("Tracing photons", taskCount);
        tbb::task::spawn_root_and_wait(tasks);
    }

    // Build KD-tree from photon list
    progress.startNewJob("Building photon KD-tree", 1);
    auto tree = KDTreeBuilder::build(photons);
    progress.signalTaskFinished();

    progress.startNewJob("Packing photon KD-tree", 1);
    //tree.pack();
    progress.signalTaskFinished();

    return tree;
}
