#pragma once
#include <optional>
#include "Ray.h"

namespace Triangle
{
	inline float getSurfaceArea(const Vector3& a, const Vector3& b, const Vector3& c)
	{
		Vector3 v1 = b - a;
		auto v1Length = v1.norm();
		Vector3 v1Normalized = v1 / v1Length;
		Vector3 v2 = c - a;

		const auto width = v1Length;
		Vector3 proj = v1Normalized * v2.dot(v1Normalized);
		const auto height = (v2 - proj).norm();
		return width * height / 2.0f;
	}

	struct TriangleIntersection
	{
		double beta, gamma, t;

		TriangleIntersection(double beta, double gamma, double t)
			: beta(beta), gamma(gamma), t(t)
		{ }
	};

	inline std::optional<TriangleIntersection> intersect(const Ray & ray, const Vector3 & a, const Vector3 & b, const Vector3 & c)
	{
		auto& d = ray.getDirection();
		Point x = a - ray.getOrigin();

		Eigen::Matrix3f p;
		p.col(0) = d;
		p.col(1) = a - b;
		p.col(2) = a - c;
		auto pDet = p.determinant();

		if (pDet == 0) //triangle is invalid
		{
			return std::nullopt;
		}

		Eigen::Matrix3f p0;
		p0.col(0) = x;
		p0.col(1) = a - b;
		p0.col(2) = a - c;
		auto p0Det = p0.determinant();
		auto t = p0Det / pDet;

		if (t < 0)
		{
			return std::nullopt;
		}

		Eigen::Matrix3f p1;
		p1.col(0) = d;
		p1.col(1) = x;
		p1.col(2) = a - c;
		auto p1Det = p1.determinant();
		auto beta = p1Det / pDet;

		if (beta < 0)
		{
			return std::nullopt;
		}

		Eigen::Matrix3f p2;
		p2.col(0) = d;
		p2.col(1) = a - b;
		p2.col(2) = x;
		auto p2Det = p2.determinant();
		auto gamma = p2Det / pDet;

		if (gamma < 0)
		{
			return std::nullopt;
		}

		if (gamma + beta > 1)
		{
			return std::nullopt;
		}

		return TriangleIntersection(beta, gamma, t);
	}
}
