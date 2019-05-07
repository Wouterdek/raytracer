#pragma once

#include <tbb/tbb.h>
#include "KDTree.h"
#include "Photon.h"
#include "scene/renderable/Scene.h"
#include "utility/ProgressMonitor.h"

class PointLightPhotonTracingTask : public tbb::task
{
public:
    using size_type = std::vector<Photon>::size_type;

    std::reference_wrapper<const Scene> scene;
    std::reference_wrapper<const PointLight> light;
    std::reference_wrapper<tbb::concurrent_vector<Photon>> photons;
    ProgressTracker& progress;
    size_type startIdx;
    size_type endIdx;
    size_type totalPhotonCount;

    PointLightPhotonTracingTask(const Scene& scene, const PointLight& light, tbb::concurrent_vector<Photon>& photons, size_type startIdx, size_type endIdx, size_type totalPhotonCount, ProgressTracker& progress)
            : scene(std::ref(scene)), light(std::ref(light)), photons(photons), startIdx(startIdx), endIdx(endIdx), totalPhotonCount(totalPhotonCount), progress(progress)
    { }

    task* execute() override;
};

class AreaLightPhotonTracingTask : public tbb::task
{
public:
    using size_type = std::vector<Photon>::size_type;

    std::reference_wrapper<const Scene> scene;
    std::reference_wrapper<const AreaLight> lightRef;
    std::reference_wrapper<tbb::concurrent_vector<Photon>> photons;
    ProgressTracker& progress;
    size_type startIdx;
    size_type endIdx;
    size_type totalPhotonCount;

    AreaLightPhotonTracingTask(const Scene& scene, const AreaLight& light, tbb::concurrent_vector<Photon>& photons, size_type startIdx, size_type endIdx, size_type totalPhotonCount, ProgressTracker& progress)
            : scene(std::ref(scene)), lightRef(std::ref(light)), photons(photons), startIdx(startIdx), endIdx(endIdx), totalPhotonCount(totalPhotonCount), progress(progress)
    { }

    task* execute() override;
};

class PhotonTracer
{
public:
    using size_type = std::vector<Photon>::size_type;

    size_type photonsPerPointLight = 1E6;
    size_type photonsPerAreaLight = 1E6;
    size_type batchSize = 1000;

    void createPhotonTracingTasks(const Scene& scene, tbb::task_list& tasks, size_type& taskCount, tbb::concurrent_vector<Photon>& photons, ProgressTracker& progress);
};