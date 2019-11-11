#include "ImageMapEnvironment.h"

#include <utility>

ImageMapEnvironment::ImageMapEnvironment(std::shared_ptr<Texture<float>> environmentMap)
        : texture(std::move(environmentMap))
{}

ImageMapEnvironment* ImageMapEnvironment::cloneImpl() const
{
    return new ImageMapEnvironment(this->texture);
}

RGB ImageMapEnvironment::getRadiance(const Scene &scene, const Vector3 &direction) const
{
    return RGB{};
}
