#include <photonmapping/Photon.h>
#include <scene/renderable/Scene.h>
#include "PluckerTreeDiagMaterial.h"

void PluckerTreeDiagMaterial::sampleTransport(TransportBuildContext &ctx) const
{
    auto& transport = ctx.getCurNode();
    transport.pathTerminationChance = 1.0;
    transport.isEmissive = true;
}

RGB PluckerTreeDiagMaterial::bsdf(const Scene &scene, const std::vector<TransportNode> &path, int curI, TransportNode &curNode, const RGB &incomingEnergy) const
{
    unsigned int maxPhotons = 1;
    std::vector<const PhotonRay*> closestRays(maxPhotons);

    float maxDist = INFINITY;
    scene.getPhotonRayMap()->FindNearestHits(curNode.hit.getHitpoint(), curNode.hit.normal, closestRays.begin(), closestRays.end(), maxDist);

    return RGB(pluckertree::Diag::visited, 0, 0);
}

std::tuple<Vector3, RGB, float> PluckerTreeDiagMaterial::interactPhoton(const SceneRayHitInfo &hit, const RGB &incomingEnergy) const
{
    return std::make_tuple(hit.ray.getDirection(), incomingEnergy, 1.0); //diffuse pass-through
}

