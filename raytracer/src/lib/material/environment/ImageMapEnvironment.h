#pragma once

#include <film/RGB.h>
#include <scene/renderable/Scene.h>
#include <material/Texture.h>
#include "IEnvironmentMaterial.h"


class ImageMapEnvironment : public IEnvironmentMaterial
{
public:
    explicit ImageMapEnvironment(std::shared_ptr<Texture<float>> environmentMap);
    RGB getRadiance(const Scene &scene, const Vector3 &direction) const override;

    double intensity = 1.0;
private:
    ImageMapEnvironment* cloneImpl() const override;
    std::shared_ptr<Texture<float>> texture;
};
