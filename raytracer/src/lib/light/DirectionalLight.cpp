#include "DirectionalLight.h"
#include "math/Transformation.h"

DirectionalLight::DirectionalLight()
    : color(RGB{ 1.0, 1.0, 1.0 }), intensity(1.0), direction(0.0, -1.0, 0.0), angle(0.0f)
{

}

void DirectionalLight::applyTransform(const Transformation &transform) {
    this->direction = transform.transform(this->direction);
    this->direction.normalize();
}

DirectionalLight *DirectionalLight::cloneImpl() const {
    return new DirectionalLight(*this);
}
