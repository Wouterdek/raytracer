#pragma once

#include "../shape/IShape.h"

class Box : public IShape
{
public:
	Box();
	Box(Point start, Point end);

	std::array<Point, 8> getCorners() const;
	Point getCentroid() const override;
	Box getAABB() const override;
	Box getAABBOfTransformed(const Transformation& transform) const;
	double getSurfaceArea() const;
	std::optional<RayHitInfo> intersect(const Ray& ray) const;
	Box merge(const Box& b) const;

//private:
	Vector3 getNormal(const Ray& ray, double t) const;
	Point start;
	Point end;
};