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
    auto latitude = std::asin(direction.y());
    auto longtitude = M_PI + std::atan2(direction.z()/std::cos(latitude), direction.x()/std::cos(latitude));

    auto x = texture->getWidth() * longtitude/(2.0f*M_PI);
    auto y = texture->getHeight() * (1.0f - (((M_PI/2.0f) + latitude)/M_PI));
    return this->texture->get(static_cast<int>(x), static_cast<int>(y)) * intensity;
}
