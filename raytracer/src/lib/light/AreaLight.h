#pragma once
#include "math/Vector3.h"
#include "film/RGB.h"
#include "utility/ICloneable.h"

class Transformation;

class AreaLight : public ICloneable<AreaLight>
{
public:
	AreaLight();

	void applyTransform(const Transformation& transform);

	Point generateRandomPoint() const;
	Vector3 getNormal() const;
	double getSurfaceArea() const;

public:
//private:
	Point a;
	Point b;
	Point c;

	RGB color;
	double intensity;

private:
	AreaLight* cloneImpl() override;
};
