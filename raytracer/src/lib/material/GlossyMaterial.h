#pragma once

#include "IMaterial.h"
#include "Texture.h"
#include <memory>

class GlossyMaterial : public IMaterial
{
public:
    GlossyMaterial();

    virtual RGB getColorFor(const SceneRayHitInfo& hit, const Scene& scene, int depth) const override;

    float roughness = 0.0f;
    std::shared_ptr<Texture> normalMap;
};