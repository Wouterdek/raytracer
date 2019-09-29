#pragma once

#include "FlatMaterial.h"

class PositionMaterial : public FlatMaterial
{
public:
    PositionMaterial();

    RGB bsdf(const Scene& scene, const std::vector<TransportNode>& path, int curI, TransportNode& curNode, const RGB& incomingEnergy) const override;

};

