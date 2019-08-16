#pragma once

#include "FlatMaterial.h"

class PositionMaterial : public FlatMaterial
{
public:
    PositionMaterial();

    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

    virtual RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
};

