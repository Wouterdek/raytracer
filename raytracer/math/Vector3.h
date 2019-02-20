#pragma once

#include <Eigen/Dense>
#include <string>

using Vector3 = Eigen::Vector3f;
using Point = Eigen::Vector3f;

std::string formatRow(const Vector3& vect);