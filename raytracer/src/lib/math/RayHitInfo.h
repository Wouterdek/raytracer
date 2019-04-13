#pragma once

#include "Ray.h"
#include "Vector3.h"
#include "Vector2.h"

class RayHitInfo
{
public:
	RayHitInfo(Ray ray, double t, Vector3 normal, Vector2 texCoord)
		: ray(std::move(ray)), t(t), normal(std::move(normal)), texCoord(std::move(texCoord))
	{
		assert(!std::isnan(t));
	}

	Point getHitpoint() const;

	Ray ray;
	double t;
	Vector3 normal;
	Vector2 texCoord;
};
