#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "shape/Box.h"

using namespace testing;

TEST(Box, TestCorners)
{
	Box box(Point(-1, -2, -3), Point(4, 5, 6));
	auto corners = box.getCorners();
	ASSERT_THAT(corners, UnorderedElementsAre(
		Point(-1, -2, -3), Point(-1, -2, 6), Point(-1, 5, -3), 
		Point(4, -2, -3), Point(-1, 5, 6), Point(4, -2, 6), 
		Point(4, 5, -3), Point(4, 5, 6))
	);
}

TEST(Box, AABB)
{
	Box box(Point(-1, -2, -3), Point(4, 5, 6));
	AABB b2 = box.getAABB();
	ASSERT_EQ(box.getStart(), b2.getStart());
	ASSERT_EQ(box.getEnd(), b2.getEnd());
}

TEST(Box, AABBTransformed)
{
	Transformation transformation = Transformation::translate(10, 10, 10);
	AABB box(Point(-1, -2, -1), Point(1, 2, 1));
	AABB b2 = box.getAABBOfTransformed(transformation);
	ASSERT_EQ(b2.getStart(), Point(9, 8, 9));
	ASSERT_EQ(b2.getEnd(), Point(11, 12, 11));
}

TEST(Box, Centroid)
{
	Box box(Point(1, 1, 1), Point(3, 3, 3));
	ASSERT_EQ(box.getCentroid(), Point(2, 2, 2));
}

TEST(Box, SurfaceArea)
{
	Box box(Point(0, 1, 2), Point(3, 3, 3));
	ASSERT_EQ(box.getSurfaceArea(), 22);
}

TEST(AABB, Merge)
{
	AABB boxA(Point(-2, -1, -1), Point(2, 1, 1));
	AABB boxB(Point(-1, 2, -3), Point(1, 3, 3));
	AABB merged = boxA.merge(boxB);
	ASSERT_EQ(merged.getStart(), Point(-2, -1, -3));
	ASSERT_EQ(merged.getEnd(), Point(2, 3, 3));
}

/*TEST(Box, Intersect)
{
	Box box(Point(1, 1, 1), Point(3, 3, 3));
	Vector3 dir = Point(3, 3, 2) - Point(1, 1, 0);
	dir.normalize();
	Ray ray(Point(0.5, 0.5, 0), dir);
	auto hit = box.intersect(ray);
	ASSERT_TRUE(hit.has_value());
	auto hitpoint = hit->getHitpoint();
	ASSERT_EQ(hitpoint, Point(1.5, 1.5, 1));
	ASSERT_EQ(hit->normal, Point(0, 0, -1));
}*/