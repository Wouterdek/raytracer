#pragma once

#include "Ray.h"
#include "Vector3.h"

class RayHitInfo
{
public:
	RayHitInfo(Ray ray, double t, Vector3 normal);

	Point getHitpoint() const;

	Ray ray;
	double t;
	Vector3 normal;
};