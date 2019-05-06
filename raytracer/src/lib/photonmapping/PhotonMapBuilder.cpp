#include "PhotonMapBuilder.h"
#include "math/OrthonormalBasis.h"
#include "math/UniformSampler.h"
#include "math/Ray.h"
#include "Photon.h"
#include "KDTree.h"
#include "KDTreeBuilder.h"
#include <vector>
#include <tbb/tbb.h>

namespace {
    thread_local std::random_device randDevice;
    std::uniform_real_distribution<float> randDist(0, 1);
};

using size_type = std::vector<Photon>::size_type;

void tracePhoton(const Scene& scene, Ray& photonRay, RGB& photonEnergy, tbb::concurrent_vector<Photon>& resultAcc)
{
    bool hasPassedDiffuse = false;
    bool hasPassedSpecular = false;
    float russianRouletteRate = 0.1f;

    while(true){
        auto hit = scene.traceRay(photonRay);

        if(!hit.has_value())
        {
            break;
        }
        auto hitpoint = hit->getHitpoint();

        // Calculate bounce/transmission/..
        auto [newPhotonRayDir, newPhotonEnergy, diffuseness] = hit->getModelNode().getData().getMaterial().interactPhoton(*hit, photonEnergy);

        bool isDiffuseTransport = diffuseness >= 0.2;
        bool isCaustic = isDiffuseTransport && hasPassedSpecular && !hasPassedDiffuse;

        // Store photon
        //TODO: maybe not store if this is a specular transport?
        resultAcc.emplace_back(hitpoint, photonRay.getDirection(), photonEnergy, isCaustic);

        // Update transport variables
        hasPassedDiffuse = hasPassedDiffuse || isDiffuseTransport;
        hasPassedSpecular = hasPassedSpecular || !isDiffuseTransport;

        photonRay = Ray(hitpoint + newPhotonRayDir * 0.0001, newPhotonRayDir);
        photonEnergy = newPhotonEnergy;

        if(randDist(randDevice) < russianRouletteRate)
        {
            break;
        }
    }
}

class PointLightPhotonTracingTask : public tbb::task
{
public:
    using Node = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    using NodePtr = std::unique_ptr<Node>;

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

    task* execute() override
    {
        // Trace photons from light source into scene
        RGB energyPerPhoton = light.get().color * (light.get().intensity / totalPhotonCount);

        for (size_type photonI = startIdx; photonI < endIdx; ++photonI) {
            auto photonDir = sampleUniformSphere(1.0);
            Ray photonRay(light.get().pos + photonDir*0.0001f, photonDir);
            RGB photonEnergy = energyPerPhoton;
            tracePhoton(scene, photonRay, photonEnergy, photons);
        }
        progress.signalTaskFinished();

        return nullptr;
    }
};

class AreaLightPhotonTracingTask : public tbb::task
{
public:
    using Node = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    using NodePtr = std::unique_ptr<Node>;

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

    task* execute() override
    {
        // Trace photons from light source into scene
        auto& light = lightRef.get();
        RGB energyPerPhoton = light.color * (light.intensity / totalPhotonCount);
        OrthonormalBasis basis((light.b - light.a).cross(light.c - light.a));

        for (size_type photonI = startIdx; photonI < endIdx; ++photonI) {
            auto photonPos = sampleUniformTriangle(light.a, light.b, light.c); //stratification?
            auto localDir = sampleUniformHemisphere(1.0);
            auto photonDir = (basis.getU() * localDir.x()) + (basis.getV() * localDir.y()) + (basis.getW() * localDir.z());

            Ray photonRay(photonPos + photonDir*0.0001f, photonDir);
            RGB photonEnergy = energyPerPhoton;
            tracePhoton(scene, photonRay, photonEnergy, photons);
        }

        progress.signalTaskFinished();

        return nullptr;
    }
};

PhotonMap PhotonMapBuilder::buildPhotonMap(const Scene& scene, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);
    std::vector<Photon> photonsVect;

    {
        tbb::concurrent_vector<Photon> photons;

        tbb::task_list tasks;
        size_type taskCount = 0;
        size_type batchSize = 1000;

        // Emit photons from all point light sources
        size_type photonsPerPointLight = 1E9;
        for(const auto& light : scene.getPointLights())
        {
            for(size_type i = 0; i < photonsPerPointLight; i += batchSize)
            {
                auto& task = *new(tbb::task::allocate_root()) PointLightPhotonTracingTask(scene, *light, photons, i, i+batchSize, photonsPerPointLight, progress);
                tasks.push_back(task);
                taskCount++;
            }
        }

        // Emit photons from all area light sources
        size_type photonsPerAreaLight = 1E6;
        for(const auto& light : scene.getAreaLights())
        {
            for(size_type i = 0; i < photonsPerAreaLight; i += batchSize)
            {
                auto& task = *new(tbb::task::allocate_root()) AreaLightPhotonTracingTask(scene, *light, photons, i, i+batchSize, photonsPerAreaLight, progress);
                tasks.push_back(task);
                taskCount++;
            }
        }

        progress.startNewJob("Tracing photons", taskCount);
        tbb::task::spawn_root_and_wait(tasks);

        photonsVect = std::vector<Photon>(photons.begin(), photons.end());
    }

    // Build KD-tree from photon list
    progress.startNewJob("Building photon KD-tree", 1);
    auto tree = KDTreeBuilder::build(photonsVect);
    progress.signalTaskFinished();

    progress.startNewJob("Packing photon KD-tree", 1);
    tree.pack();
    progress.signalTaskFinished();

    return tree;
}
