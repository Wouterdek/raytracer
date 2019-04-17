#pragma once

#include "IMaterial.h"
#include <memory>

class MixMaterial : public IMaterial {
public:
    RGB getColorFor(const SceneRayHitInfo &hit, const Scene &scene, int depth) const override;

    double mixFactor;
    std::shared_ptr<IMaterial> first;
    std::shared_ptr<IMaterial> second;
};
