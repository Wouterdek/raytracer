#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "math/Transformation.h"

using namespace testing;

TEST(Quaternion, TestAroundX)
{
    //90° around +X
    auto rot = Transformation::rotateQuaternion(0.7071068, 0, 0, 0.7071068);

    auto result = rot.transform(Vector3(1,0,0));
    ASSERT_FLOAT_EQ(result.x(), 1);
    ASSERT_FLOAT_EQ(result.y(), 0);
    ASSERT_FLOAT_EQ(result.z(), 0);

    result = rot.transform(Vector3(0,1,0));
    ASSERT_FLOAT_EQ(result.x(), 0);
    ASSERT_FLOAT_EQ(result.y(), 0);
    ASSERT_FLOAT_EQ(result.z(), 1);

    result = rot.transform(Vector3(0,0,1));
    ASSERT_FLOAT_EQ(result.x(), 0);
    ASSERT_FLOAT_EQ(result.y(), -1);
    ASSERT_FLOAT_EQ(result.z(), 0);
}

TEST(Quaternion, TestAroundY)
{
    //90° around +Y
    auto rot = Transformation::rotateQuaternion(0, 0.7071068, 0, 0.7071068);

    auto result = rot.transform(Vector3(1,0,0));
    ASSERT_FLOAT_EQ(result.x(), 0);
    ASSERT_FLOAT_EQ(result.y(), 0);
    ASSERT_FLOAT_EQ(result.z(), -1);

    result = rot.transform(Vector3(0,0,1));
    ASSERT_FLOAT_EQ(result.x(), 1);
    ASSERT_FLOAT_EQ(result.y(), 0);
    ASSERT_FLOAT_EQ(result.z(), 0);
}

TEST(Quaternion, TestAroundZ)
{
    //90° around +Y
    auto rot = Transformation::rotateQuaternion(0, 0, 0.7071068, 0.7071068);

    auto result = rot.transform(Vector3(1,0,0));
    ASSERT_FLOAT_EQ(result.x(), 0);
    ASSERT_FLOAT_EQ(result.y(), 1);
    ASSERT_FLOAT_EQ(result.z(), 0);

    result = rot.transform(Vector3(0,1,0));
    ASSERT_FLOAT_EQ(result.x(), -1);
    ASSERT_FLOAT_EQ(result.y(), 0);
    ASSERT_FLOAT_EQ(result.z(), 0);
}

TEST(Quaternion, TestAroundZX)
{
    //45° around +Z, then -45° around +Y
    auto rot = Transformation::rotateQuaternion(0.3535533, -0.3535533, 0.1464466, 0.8535534);

    auto result = rot.transform(Vector3(1,0,0));
    ASSERT_FLOAT_EQ(result.x(), 0.707106888);
    ASSERT_FLOAT_EQ(result.y(), 0.00000011481512);
    ASSERT_FLOAT_EQ(result.z(), 0.707106888);
}