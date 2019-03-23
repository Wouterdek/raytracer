#include "AreaLight.h"
#include "math/Transformation.h"
#include "math/Triangle.h"
#include <random>

thread_local std::random_device random;
std::uniform_real_distribution<double> dist(0, 1);

AreaLight::AreaLight()
	: color(RGB{ 1.0, 1.0, 1.0 }), intensity(10.0), a(0, 0, 0), b(1, 0, 0), c(0, 1, 0)
{
}

void AreaLight::applyTransform(const Transformation& transform)
{
	this->a = transform.transform(this->a);
	this->b = transform.transform(this->b);
	this->c = transform.transform(this->c);
}

Point AreaLight::generateRandomPoint() const
{
	auto u = dist(random);
	auto v = dist(random);

	return this->a + u*(c - a) + v*(b - a);
}

Vector3 AreaLight::getNormal() const
{
	auto normal = (b - a).cross(c - a);
	normal.normalize();
	return normal;
}

double AreaLight::getSurfaceArea() const
{
	return Triangle::getSurfaceArea(a, b, c);
}

AreaLight* AreaLight::cloneImpl()
{
	return new AreaLight(*this);
}
