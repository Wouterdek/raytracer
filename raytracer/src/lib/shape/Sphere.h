#pragma once

#include "IShape.h"
#include "../math/Transformation.h"

class Sphere : public IShape
{
public:
	Sphere(Transformation transform);

	bool intersect(const Ray& ray) const override;

private:
	Transformation transform;
};