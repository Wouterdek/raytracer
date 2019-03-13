#include "TriangleMesh.h"
#include <Eigen/Dense>
#include <numeric>

TriangleMesh::TriangleMesh(std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices, std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices)
	: data(std::make_shared<TriangleMeshData>()), beginIdx(0), endIdx(vertexIndices.size())
{
	data->vertices = std::move(vertices);
	data->vertexIndices = std::move(vertexIndices);
	data->normals = std::move(normals);
	data->normalIndices = std::move(normalIndices);
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

	TriangleIntersection intersection(beta, gamma, t);
	return intersection;
}

std::optional<RayHitInfo> TriangleMesh::intersect(const Ray& ray) const
{
	for(size_type triangleI = this->beginIdx; triangleI < this->endIdx; triangleI++)
	{
		const auto& indices = data->vertexIndices[triangleI];
		auto& a = data->vertices[indices[0]];
		auto& b = data->vertices[indices[1]];
		auto& c = data->vertices[indices[2]];

		auto intersection = intersectTriangle(ray, a, b, c);
		if(intersection.has_value())
		{
			const auto& normalIndices = data->normalIndices[triangleI];
			auto& aNormal = data->normals[normalIndices[0]];
			auto& bNormal = data->normals[normalIndices[1]];
			auto& cNormal = data->normals[normalIndices[2]];

			double alfa = 1.0 - intersection->beta - intersection->gamma;
			Vector3 normal = (alfa * aNormal) + (intersection->beta * bNormal) + (intersection->gamma * cNormal);
			normal.normalize();
			return RayHitInfo(ray, intersection->t, normal);
		}
	}

	return std::nullopt;
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
	const Point newStart(startX, startY, startZ);
	const Point newEnd(endX, endY, endZ);

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

//From https://stackoverflow.com/questions/17074324/how-can-i-sort-two-vectors-in-the-same-way-with-criteria-that-uses-only-one-of
//Wouldn't be needed if C++ supported SOA sorting...
template <typename T>
void apply_permutation_in_place(
	const std::vector<uint32_t>& p,
	int stride,
	typename std::vector<T>::iterator vec1,
	typename std::vector<T>::iterator vec2
)
{
	std::vector<bool> done(p.size());

	auto apply_permutation_in_place_impl = [](
		const std::vector<uint32_t>& p,
		int elementsPerItem,
		typename std::vector<T>::iterator vec,
		std::vector<bool>& done)
	{
		for (uint32_t i = 0; i < p.size(); i++)
		{
			if (done[i])
			{
				continue;
			}
			done[i] = true;
			uint32_t prev_j = i;
			uint32_t j = p[i];
			while (i != j)
			{
				std::swap_ranges(
					vec + (elementsPerItem * prev_j),
					vec + (elementsPerItem * (prev_j + 1)),
					vec + (elementsPerItem * j)
				);
				done[j] = true;
				prev_j = j;
				j = p[j];
			}
		}
	};

	apply_permutation_in_place_impl(p, stride, vec1, done);
	std::fill(done.begin(), done.end(), false);
	apply_permutation_in_place_impl(p, stride, vec2, done);
}

void TriangleMesh::sortByCentroid(Axis axis)
{
	std::vector<uint32_t> indices(this->count());
	std::iota(indices.begin(), indices.end(), 0);

	std::sort(indices.begin(), indices.end(), [axis = axis, this](const uint32_t a, const uint32_t b)
	{
		auto c1 = getCentroid(a);
		auto c2 = getCentroid(b);
		return c1[static_cast<int>(axis)] < c2[static_cast<int>(axis)];
	});

	apply_permutation_in_place<std::array<uint32_t, 3>>(indices, 1, data->vertexIndices.begin() + this->beginIdx, data->normalIndices.begin() + this->beginIdx);
}

std::optional<RayHitInfo> TriangleMesh::traceRay(const Ray& ray) const
{
	return this->intersect(ray);
}

TriangleMesh* TriangleMesh::cloneImpl()
{
	return new TriangleMesh(*this);
}
