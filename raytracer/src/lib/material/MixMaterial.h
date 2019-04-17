#pragma once

#include "IMaterial.h"
#include <memory>

class MixMaterial : public IMaterial {
public:
    RGB getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;

    double mixFactor = 0.0;
    std::shared_ptr<IMaterial> first;
    std::shared_ptr<IMaterial> second;
};
