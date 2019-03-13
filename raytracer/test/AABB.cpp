#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "shape/Box.h"

using namespace testing;

TEST(AABB, TestCorners)
{
	Box box(Point(-1, -2, -3), Point(4, 5, 6));
	auto corners = box.getCorners();
	ASSERT_THAT(corners, UnorderedElementsAre(
		Point(-1, -2, -3), Point(-1, -2, 6), Point(-1, 5, -3), 
		Point(4, -2, -3), Point(-1, 5, 6), Point(4, -2, 6), 
		Point(4, 5, -3), Point(4, 5, 6))
	);
}