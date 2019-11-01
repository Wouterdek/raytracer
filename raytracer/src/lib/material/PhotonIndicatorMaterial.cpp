#include "PhotonIndicatorMaterial.h"
#include "photonmapping/PhotonMap.h"
#include "scene/renderable/Scene.h"

void PhotonIndicatorMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.pathTerminationChance = 1.0;
    transport.isEmissive = true;
}

RGB PhotonIndicatorMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode &curNode, const RGB &incomingEnergy) const
{
    std::vector<const Photon*> photons(1);

    auto dir = -curNode.hit.ray.getDirection();
    auto [nbPhotonsFound, maxDist] = scene.getPhotonMap()->getElementsNearestTo(curNode.hit.getHitpoint(), photons.size(), 1E9, [dir](const Photon& photon){
        return dir.dot(photon.surfaceNormal) >= 0;
    }, photons);

    return RGB(maxDist);
}

std::tuple<Vector3, RGB, float> PhotonIndicatorMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return std::make_tuple(hit.ray.getDirection(), incomingEnergy, 1.0); //diffuse pass-through
}
