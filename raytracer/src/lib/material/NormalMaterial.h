#pragma once

#include "FlatMaterial.h"

class NormalMaterial : public FlatMaterial
{
public:
	NormalMaterial();

    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

	RGB getTotalRadianceTowards(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;
};