#include "MixMaterial.h"
#include <random>

namespace {
    thread_local std::random_device randDevice;
    std::uniform_real_distribution<float> randDist(0, 1);
};

RGB MixMaterial::getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const {
    if(randDist(randDevice) > mixFactor)
    {
        return this->first->getTotalRadianceTowards(hit, scene, depth);
    }
    else
    {
        return this->second->getTotalRadianceTowards(hit, scene, depth);
    }

    /*RGB contribA;
    if(mixFactor < 1){
        contribA = this->first->getTotalRadianceTowards(hit, scene, depth);
    }

    RGB contribB;
    if(mixFactor > 0){
        contribB = this->second->getTotalRadianceTowards(hit, scene, depth);
    }

    return (contribA * (1.0-mixFactor)) + (contribB * mixFactor);*/
}

std::tuple<Vector3, RGB, float> MixMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    if(randDist(randDevice) > mixFactor)
    {
        return this->first->interactPhoton(hit, incomingEnergy);
    }
    else
    {
        return this->second->interactPhoton(hit, incomingEnergy);
    }
}
