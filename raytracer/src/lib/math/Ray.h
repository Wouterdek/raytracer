#pragma once

#include <string>
#include <sstream>
#include "Vector3.h"

#include <array>

class Ray {
public:
	Ray(Point origin, Vector3 dir);

	const Point& getOrigin() const
	{
		return this->origin;
	}

	const Vector3 getDirection() const
	{
		return this->direction;
	}

private:
	Point origin;
	Vector3 direction;
};

std::ostream& operator<<(std::ostream& in, const Ray& ray);
