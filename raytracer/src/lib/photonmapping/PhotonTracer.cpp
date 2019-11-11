#include <math/OrthonormalBasis.h>
#include "PhotonTracer.h"
#include "math/Sampler.h"
#include "math/FastRandom.h"
#include "utility/Task.h"

void tracePhoton(const Scene& scene, Ray& photonRay, RGB photonEnergy, PhotonMapMode mode, PhotonList& resultAcc)
{
    bool hasPassedDiffuse = false;
    bool hasPassedSpecular = false;
    //while(true){
    int maxDepth = 12;
    for(int i = 0; i < maxDepth; ++i){
        auto hit = scene.traceRay(photonRay);

        if(!hit.has_value())
        {
            break;
        }
        auto hitpoint = hit->getHitpoint();

        // Calculate bounce/transmission/..
        auto [newPhotonRayDir, newPhotonEnergy, diffuseness] = hit->getModelNode().getData().getMaterial().interactPhoton(*hit, photonEnergy);

        bool isDiffuseTransport = diffuseness >= 0.2;
        bool isCaustic = isDiffuseTransport && hasPassedSpecular /*&& !hasPassedDiffuse*/;

        // Store photon
        if((mode == PhotonMapMode::caustics && isCaustic) ||
           (mode == PhotonMapMode::full && isDiffuseTransport))
        {
            resultAcc.emplace_back(hitpoint, photonRay.getDirection(), hit->normal, photonEnergy, isCaustic);
        }

        // Update transport variables
        hasPassedDiffuse = hasPassedDiffuse || isDiffuseTransport;
        hasPassedSpecular = hasPassedSpecular || !isDiffuseTransport;

        photonRay = Ray(hitpoint + newPhotonRayDir * 0.00005, newPhotonRayDir);

        photonEnergy = newPhotonEnergy;

        /*auto reflectance = newPhotonEnergy.divide(photonEnergy+RGB{1E-6, 1E-6, 1E-6});
        auto avgReflectance = (reflectance.getRed() + reflectance.getGreen() + reflectance.getBlue())/3.0f;
        photonEnergy = newPhotonEnergy.divide(avgReflectance);

        if(Rand::unit() > avgReflectance)
        {
            break;
        }*/
    }
}

class PointLightPhotonTracingTask : public Task
{
public:
    using size_type = std::vector<Photon>::size_type;

    std::reference_wrapper<const Scene> scene;
    std::reference_wrapper<const PointLight> light;
    std::reference_wrapper<PhotonList> photons;
    ProgressTracker& progress;
    PhotonMapMode mode;
    size_type startIdx;
    size_type endIdx;
    size_type totalPhotonCount;

    PointLightPhotonTracingTask(const Scene& scene, const PointLight& light, PhotonList& photons,
                                PhotonMapMode mode, size_type startIdx, size_type endIdx, size_type totalPhotonCount, ProgressTracker& progress)
            : scene(std::ref(scene)), light(std::ref(light)), photons(photons), mode(mode), startIdx(startIdx),
              endIdx(endIdx), totalPhotonCount(totalPhotonCount), progress(progress)
    { }

    void execute() override
    {
        // Trace photons from light source into scene
        RGB energyPerPhoton = light.get().color * (light.get().intensity / (double)totalPhotonCount);

        for (size_type photonI = startIdx; photonI < endIdx; ++photonI) {
            auto photonDir = sampleUniformSphere(1.0);

            Ray photonRay(light.get().pos, photonDir);
            tracePhoton(scene, photonRay, energyPerPhoton, mode, photons);
        }
        progress.signalTaskFinished();
    }
};

class AreaLightPhotonTracingTask : public Task
{
public:
    using size_type = std::vector<Photon>::size_type;

    std::reference_wrapper<const Scene> scene;
    std::reference_wrapper<const AreaLight> lightRef;
    std::reference_wrapper<PhotonList> photons;
    PhotonMapMode mode;
    ProgressTracker& progress;
    size_type startIdx;
    size_type endIdx;
    size_type totalPhotonCount;

    AreaLightPhotonTracingTask(const Scene& scene, const AreaLight& light, PhotonList& photons,
                               PhotonMapMode mode, size_type startIdx, size_type endIdx, size_type totalPhotonCount, ProgressTracker& progress)
            : scene(std::ref(scene)), lightRef(std::ref(light)), photons(photons), mode(mode), startIdx(startIdx),
              endIdx(endIdx), totalPhotonCount(totalPhotonCount), progress(progress)
    { }

    void execute() override
    {
        // Trace photons from light source into scene
        const auto& light = lightRef.get();
        auto lightEnergy = light.color * light.intensity;
        RGB energyPerPhoton = lightEnergy.divide(totalPhotonCount);
        OrthonormalBasis basis((light.b - light.a).cross(light.c - light.a));

        for (size_type photonI = startIdx; photonI < endIdx; ++photonI) {
            auto photonPos = light.generateStratifiedJitteredRandomPoint(endIdx-startIdx, photonI-startIdx);
            auto localDir = mapSampleToCosineWeightedHemisphere(Rand::unit(), Rand::unit(), 1.0);
            auto photonDir = (basis.getU() * localDir.x()) + (basis.getV() * localDir.y()) + (basis.getW() * localDir.z());

            Ray photonRay(photonPos + photonDir*0.0001f, photonDir);
            tracePhoton(scene, photonRay, energyPerPhoton, mode, photons);
        }

        progress.signalTaskFinished();
    }
};

void PhotonTracer::tracePhotons(const Scene &scene, PhotonList& photons, ProgressMonitor progressMon)
{
    ProgressTracker progress(progressMon);

    std::vector<std::unique_ptr<Task>> tasks{};
    size_type taskCount = 0;

    taskCount = 0;

    // Emit photons from all point light sources
    for(const auto& light : scene.getPointLights())
    {
        for(size_type i = 0; i < photonsPerPointLight; i += batchSize)
        {
            auto endIdx = std::min(i+batchSize, photonsPerPointLight);
            tasks.push_back(std::make_unique<PointLightPhotonTracingTask>(scene, *light, photons, mode, i, endIdx, photonsPerPointLight, progress));
            taskCount++;
        }
        auto remainingPhotons = photonsPerPointLight % batchSize;
        if(remainingPhotons > 0)
        {
            auto startIdx = photonsPerPointLight - remainingPhotons;
            auto endIdx = photonsPerPointLight;
            tasks.push_back(std::make_unique<PointLightPhotonTracingTask>(scene, *light, photons, mode, startIdx, endIdx, photonsPerPointLight, progress));
            taskCount++;
        }
    }

    // Emit photons from all area light sources
    for(const auto& light : scene.getAreaLights())
    {
        for(size_type i = 0; i < photonsPerAreaLight; i += batchSize)
        {
            auto endIdx = std::min(i+batchSize, photonsPerAreaLight);
            tasks.push_back(std::make_unique<AreaLightPhotonTracingTask>(scene, *light, photons, mode, i, endIdx, photonsPerAreaLight, progress));
            taskCount++;
        }
        auto remainingPhotons = photonsPerAreaLight % batchSize;
        if(remainingPhotons > 0)
        {
            auto startIdx = photonsPerAreaLight - remainingPhotons;
            auto endIdx = photonsPerAreaLight;
            tasks.push_back(std::make_unique<AreaLightPhotonTracingTask>(scene, *light, photons, mode, startIdx, endIdx, photonsPerAreaLight, progress));
            taskCount++;
        }
    }

    progress.startNewJob("Tracing photons", taskCount);
    Task::runTasks(tasks);
}
