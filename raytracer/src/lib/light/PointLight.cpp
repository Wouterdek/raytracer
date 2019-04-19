#include "PointLight.h"
#include "film/RGB.h"
#include "math/Transformation.h"

PointLight::PointLight()
	: color(RGB{ 1.0, 1.0, 1.0 }), intensity(1.0)
{
}

void PointLight::applyTransform(const Transformation& transform)
{
	this->pos = transform.transform(this->pos);
}

PointLight* PointLight::cloneImpl() const
{
	return new PointLight(*this);
}
