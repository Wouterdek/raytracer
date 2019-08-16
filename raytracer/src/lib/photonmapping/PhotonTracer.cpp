#include <math/OrthonormalBasis.h>
#include "PhotonTracer.h"
#include "math/UniformSampler.h"

namespace {
    thread_local std::random_device randDevice;
    std::uniform_real_distribution<float> randDist(0, 1);
};

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
        if(isCaustic)
        {
            resultAcc.emplace_back(hitpoint, photonRay.getDirection(), hit->normal, photonEnergy, isCaustic);
        }

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

tbb::task *PointLightPhotonTracingTask::execute()
{
    // Trace photons from light source into scene
    RGB energyPerPhoton = light.get().color * (light.get().intensity / (float)totalPhotonCount);

    for (size_type photonI = startIdx; photonI < endIdx; ++photonI) {
        auto photonDir = sampleUniformSphere(1.0);
        Ray photonRay(light.get().pos + photonDir*0.0001f, photonDir);
        RGB photonEnergy = energyPerPhoton;
        tracePhoton(scene, photonRay, photonEnergy, photons);
    }
    progress.signalTaskFinished();

    return nullptr;
}

tbb::task *AreaLightPhotonTracingTask::execute()
{
    // Trace photons from light source into scene
    auto& light = lightRef.get();
    RGB energyPerPhoton = light.color * (light.intensity / (float)totalPhotonCount);
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

void PhotonTracer::createPhotonTracingTasks(const Scene &scene, tbb::task_list &tasks, PhotonTracer::size_type &taskCount,
                                       tbb::concurrent_vector<Photon> &photons, ProgressTracker& progress)
{
    taskCount = 0;

    // Emit photons from all point light sources
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
    for(const auto& light : scene.getAreaLights())
    {
        for(size_type i = 0; i < photonsPerAreaLight; i += batchSize)
        {
            auto& task = *new(tbb::task::allocate_root()) AreaLightPhotonTracingTask(scene, *light, photons, i, i+batchSize, photonsPerAreaLight, progress);
            tasks.push_back(task);
            taskCount++;
        }
    }
}
