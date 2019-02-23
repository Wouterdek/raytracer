#pragma once

#include "../math/Ray.h"

class IShape
{
public:
	virtual ~IShape(){};
	virtual bool intersect(const Ray& ray) const = 0;
};