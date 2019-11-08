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
		float beta, gamma;
		double t;
	};

#ifdef __linux__
#define FORCE_TOTAL_INLINE __attribute__((always_inline)) __attribute__((flatten))
#elif WIN32
#define FORCE_TOTAL_INLINE __forceinline
#else
#define FORCE_TOTAL_INLINE
#endif

    FORCE_TOTAL_INLINE inline bool intersect(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c, TriangleIntersection& result) noexcept
	{
		auto& d = ray.getDirection();
		Point x = a - ray.getOrigin();
        Vector3 e1 = a-b;
        Vector3 e2 = a-c;

		Eigen::Matrix3f mat;
		mat.row(0) = d;
		mat.row(1) = e1;
		mat.row(2) = e2;
		auto pDet = mat.determinant();

		//Vector3 e1CrossE2 = e1.cross(e2);
		//float pDet = d.dot(e1CrossE2);
		if (pDet == 0) //triangle is invalid
		{
			return false;
		}
        auto invDet = 1.0f/pDet;

        mat.row(0) = x;
        mat.row(1) = e1;
        mat.row(2) = e2;
		auto p0Det = mat.determinant();
        float t = p0Det * invDet;
        //float t = x.dot(e1CrossE2) * invDet;

		if (t < 0)
		{
			return false;
		}

        mat.row(0) = d;
        mat.row(1) = x;
        mat.row(2) = e2;
		auto p1Det = mat.determinant();
        float beta = p1Det * invDet;
        //Vector3 dCrossX = d.cross(x);
        //float beta = dCrossX.dot(e2) * invDet;

		if (beta < 0)
		{
			return false;
		}

        mat.row(0) = d;
        mat.row(1) = e1;
        mat.row(2) = x;
		auto p2Det = mat.determinant();
        float gamma = p2Det * invDet;
        //float gamma = -dCrossX.dot(e1) * invDet;

		if (gamma < 0)
		{
			return false;
		}

		if (gamma + beta > 1)
		{
			return false;
		}

        result.beta = beta;
		result.gamma = gamma;
		result.t = t;
		return true;
	}
}
