#include "TriangleMesh.h"
#include <Eigen/Dense>

TriangleMesh::TriangleMesh(std::vector<Point> vertices, std::vector<uint32_t> vertexIndices, std::vector<Vector3> normals, std::vector<uint32_t> normalIndices)
	: vertices(std::move(vertices)), vertexIndices(std::move(vertexIndices)), normals(std::move(normals)), normalIndices(std::move(normalIndices))
{ }

struct TriangleIntersection
{
	double beta, gamma, t;

	TriangleIntersection(double beta, double gamma, double t)
		: beta(beta), gamma(gamma), t(t)
	{ }
};

std::optional<TriangleIntersection> intersectTriangle(const Ray& ray, const Vector3& a, const Vector3& b, const Vector3& c)
{
	auto& d = ray.getDirection();
	Point x = a - ray.getOrigin();

	Eigen::Matrix3f p;
	p.col(0) = d;
	p.col(1) = a - b;
	p.col(2) = a - c;
	auto pDet = p.determinant();

	if(pDet == 0) //triangle is invalid
	{
		return std::nullopt;
	}

	Eigen::Matrix3f p0;
	p0.col(0) = x;
	p0.col(1) = a - b;
	p0.col(2) = a - c;
	auto p0Det = p0.determinant();
	auto t = p0Det / pDet;

	if(t < 0)
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

	if(gamma + beta > 1)
	{
		return std::nullopt;
	}

	TriangleIntersection intersection(beta, gamma, t);
	return intersection;
}

std::optional<RayHitInfo> TriangleMesh::intersect(const Ray& ray, const Transformation& transform) const
{
	using index = std::vector<Vector3>::size_type;

	auto inverseRay = transform.transformInverse(ray);
	for(index i = 0; i < this->vertexIndices.size(); i += 3)
	{
		auto& a = this->vertices[this->vertexIndices[i]];
		auto& b = this->vertices[this->vertexIndices[i+1]];
		auto& c = this->vertices[this->vertexIndices[i+2]];

		auto intersection = intersectTriangle(inverseRay, a, b, c);
		if(intersection.has_value())
		{
			auto& aNormal = this->normals[this->normalIndices[i]];
			auto& bNormal = this->normals[this->normalIndices[i + 1]];
			auto& cNormal = this->normals[this->normalIndices[i + 2]];

			double alfa = 1.0 - intersection->beta - intersection->gamma;
			Vector3 normal = (alfa * aNormal) + (intersection->beta * bNormal) + (intersection->gamma * cNormal);
			Vector3 transformedNormal = transform.transformNormal(normal);
			transformedNormal.normalize();
			return RayHitInfo(ray, intersection->t, transformedNormal);
		}
	}

	return std::nullopt;
}
