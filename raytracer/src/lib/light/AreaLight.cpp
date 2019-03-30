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
	auto u = 1.0 - sqrt(dist(random));
	auto v = (1.0 - u) * dist(random);

	return this->a + u*(c - a) + v*(b - a);
}

Point AreaLight::generateStratifiedJitteredRandomPoint(int level, int i) const
{
	int n = sqrt(level);
	Vector3 v1 = (c - a) / n;
	Vector3 v2 = (b - a) / n;

	auto u = 1.0 - sqrt(dist(random));
	auto v = (1.0 - u) * dist(random);

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

AreaLight* AreaLight::cloneImpl()
{
	return new AreaLight(*this);
}
