#pragma once

#include <string>
#include <sstream>
#include "Vector3.h"

class Ray {
public:
	const Point origin;
	const Vector3 direction;

	Ray(const Point origin, const Vector3 dir);
};

std::ostream& operator<<(std::ostream& in, const Ray& ray);