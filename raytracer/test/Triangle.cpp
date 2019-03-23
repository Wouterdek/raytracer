#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <math/Triangle.h>

using namespace testing;

TEST(Triangle, SurfaceArea)
{
	Vector3 a(1, 1, 1);
	Vector3 b(3, 1, 1);
	Vector3 c(1, 2, 1);
	auto surface = Triangle::getSurfaceArea(a, b, c);
	ASSERT_FLOAT_EQ(surface, 1);
}

TEST(Triangle, SurfaceArea2)
{
	Vector3 a(0, 1, 0);
	Vector3 b(-1, 1, 1);
	Vector3 c(2, 1, 2);
	auto surface = Triangle::getSurfaceArea(a, b, c);
	ASSERT_FLOAT_EQ(surface, 2);
}

