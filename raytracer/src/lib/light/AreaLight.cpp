#include "AreaLight.h"

#include <cmath>
#include "math/Transformation.h"
#include "math/Triangle.h"
#include "math/UniformSampler.h"
#include "math/FastRandom.h"

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
    return sampleUniformTriangle(a, b, c);
}

Point AreaLight::generateStratifiedJitteredRandomPoint(int level, int i) const noexcept
{
    assert(i < level);

	int n = sqrt(level);
	Vector3 v1 = (c - a) / n;
	Vector3 v2 = (b - a) / n;

	auto u = 1.0f - std::sqrt(Rand::unit());
	auto v = (1.0f - u) * Rand::unit();

	auto y = i / n;
	auto x = i % n;

	return this->a + ((u+x) * v1) + ((v+y) * v2);
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

AreaLight* AreaLight::cloneImpl() const
{
	return new AreaLight(*this);
}
