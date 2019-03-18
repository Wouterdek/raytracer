#include "TriangleMesh.h"
#include <Eigen/Dense>
#include <numeric>
#include "utility/soa_sort.h"

TriangleMesh::TriangleMesh(
	std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices, 
	std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices,
	std::vector<Vector2> texCoords, std::vector<std::array<uint32_t, 3>> texCoordIndices)
	: data(std::make_shared<TriangleMeshData>()), beginIdx(0), endIdx(vertexIndices.size())
{
	data->vertices = std::move(vertices);
	data->vertexIndices = std::move(vertexIndices);
	data->normals = std::move(normals);
	data->normalIndices = std::move(normalIndices);
	data->texCoords = std::move(texCoords);
	data->texCoordIndices = std::move(texCoordIndices);
}

TriangleMesh::TriangleMesh(std::shared_ptr<TriangleMeshData> data, size_type begin, size_type end)
	: data(std::move(data)), beginIdx(begin), endIdx(end)
{
}

Point TriangleMesh::getCentroid() const
{
	if (!this->centroid.has_value())
	{
		const auto count = this->count();
		Point newCentroid = this->getCentroid(0) / count;
		for (size_type i = 1; i < count; i++)
		{
			newCentroid += this->getCentroid(i) / count;
		}
		centroid = newCentroid;
	}

	return this->centroid.value();
}

Box TriangleMesh::getAABB() const
{
	if(!this->aabb.has_value())
	{
		auto newAABB = this->getAABB(0);
		const auto count = this->count();
		for(size_type i = 1; i < count; i++)
		{
			newAABB = newAABB.merge(this->getAABB(i));
		}
		aabb = newAABB;
	}

	return this->aabb.value();
}

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

	return TriangleIntersection(beta, gamma, t);
}

std::optional<RayHitInfo> TriangleMesh::intersect(const Ray& ray) const
{
	std::optional<RayHitInfo> bestHit;

	for(size_type triangleI = this->beginIdx; triangleI < this->endIdx; triangleI++)
	{
		const auto& indices = data->vertexIndices[triangleI];
		auto& a = data->vertices[indices[0]];
		auto& b = data->vertices[indices[1]];
		auto& c = data->vertices[indices[2]];

		auto intersection = intersectTriangle(ray, a, b, c);
		if(intersection.has_value() && (!bestHit.has_value() || bestHit->t > intersection->t))
		{
			double alfa = 1.0 - intersection->beta - intersection->gamma;

			const auto& normalIndices = data->normalIndices[triangleI];
			auto& aNormal = data->normals[normalIndices[0]];
			auto& bNormal = data->normals[normalIndices[1]];
			auto& cNormal = data->normals[normalIndices[2]];
			Vector3 normal = (alfa * aNormal) + (intersection->beta * bNormal) + (intersection->gamma * cNormal);

			const auto& texCoordIndices = data->texCoordIndices[triangleI];
			auto& aTexCoord = data->texCoords[texCoordIndices[0]];
			auto& bTexCoord = data->texCoords[texCoordIndices[1]];
			auto& cTexCoord = data->texCoords[texCoordIndices[2]];
			Vector2 texcoord = (alfa * aTexCoord) + (intersection->beta * bTexCoord) + (intersection->gamma * cTexCoord);

			bestHit = RayHitInfo(ray, intersection->t, normal, texcoord);
		}
	}

	return bestHit;
}

Box TriangleMesh::getAABB(size_type index) const
{
	const auto& indices = data->vertexIndices[this->beginIdx + index];
	Point p1 = data->vertices[indices[0]];
	Point p2 = data->vertices[indices[1]];
	Point p3 = data->vertices[indices[2]];

	const auto[startX, endX] = std::minmax({ p1.x(), p2.x(), p3.x() });
	const auto[startY, endY] = std::minmax({ p1.y(), p2.y(), p3.y() });
	const auto[startZ, endZ] = std::minmax({ p1.z(), p2.z(), p3.z() });
	const Point newStart(startX - 1E-4, startY - 1E-4, startZ - 1E-4);
	const Point newEnd(endX + 1E-4, endY + 1E-4, endZ + 1E-4);

	return Box(newStart, newEnd);
}

TriangleMesh::size_type TriangleMesh::count() const
{
	return this->endIdx - this->beginIdx;
}

std::pair<IShapeList<RayHitInfo>*, IShapeList<RayHitInfo>*> TriangleMesh::splitImpl(size_type leftSideElemCount) const
{
	auto m1 = new TriangleMesh(this->data, this->beginIdx, this->beginIdx + leftSideElemCount);
	auto m2 = new TriangleMesh(this->data, this->beginIdx + leftSideElemCount, this->endIdx);
	return std::make_pair(m1, m2);
}

Point TriangleMesh::getCentroid(size_type index) const
{
	const auto& indices = data->vertexIndices[this->beginIdx + index];
	const Point p1 = data->vertices[indices[0]];
	const Point p2 = data->vertices[indices[1]];
	const Point p3 = data->vertices[indices[2]];
	return (p1 + p2 + p3)/3;
}

void TriangleMesh::sortByCentroid(Axis axis)
{
	soa_sort::sort_cmp<std::vector<std::array<uint32_t, 3>>::iterator>(data->vertexIndices.begin() + this->beginIdx, data->vertexIndices.begin() + this->endIdx, [axis = axis, this](const auto a, const auto b)
		{
			Point c1;
			{
				const Point p1 = data->vertices[a[0]];
				const Point p2 = data->vertices[a[1]];
				const Point p3 = data->vertices[a[2]];
				c1 = (p1 + p2 + p3) / 3;
			}

			Point c2;
			{
				const Point p1 = data->vertices[b[0]];
				const Point p2 = data->vertices[b[1]];
				const Point p3 = data->vertices[b[2]];
				c2 = (p1 + p2 + p3) / 3;
			}
			
			return c1[static_cast<int>(axis)] < c2[static_cast<int>(axis)];
		}, data->normalIndices.begin() + this->beginIdx);
}

std::optional<RayHitInfo> TriangleMesh::traceRay(const Ray& ray) const
{
	return this->intersect(ray);
}

TriangleMesh* TriangleMesh::cloneImpl()
{
	return new TriangleMesh(*this);
}
