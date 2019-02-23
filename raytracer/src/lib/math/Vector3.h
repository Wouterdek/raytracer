#pragma once

#include <Eigen/Dense>
#include <string>

struct Point : public Eigen::Vector3f
{
	using Eigen::Vector3f::Vector3f;
};

struct Vector3 : public Eigen::Vector3f
{
	using Eigen::Vector3f::Vector3f;
};

//using Vector3 = Eigen::Vector3f;
//using Point = Eigen::Vector3f;

std::string formatRow(const Vector3& vect);