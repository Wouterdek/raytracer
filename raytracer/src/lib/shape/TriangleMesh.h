#pragma once

#include "IShape.h"
#include "math/Vector3.h"
#include "shape/list/IShapeList.h"
#include "math/Vector2.h"

struct TriangleMeshData
{
	std::vector<Point> vertices;
	std::vector<std::array<uint32_t, 3>> vertexIndices;
	std::vector<Vector3> normals;
	std::vector<std::array<uint32_t, 3>> normalIndices;
	std::vector<Vector2> texCoords;
	std::vector<std::array<uint32_t, 3>> texCoordIndices;
};

class TriangleMesh : public IShape, public IShapeList<RayHitInfo>
{
public:
	TriangleMesh(std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices,
		std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices,
		std::vector<Vector2> texCoords, std::vector<std::array<uint32_t, 3>> texCoordIndices);
	TriangleMesh(std::shared_ptr<TriangleMeshData> data, size_type begin, size_type end);

	Point getCentroid() const override;
	AABB getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const override;

	AABB getAABB(size_type index) const override;
	size_type count() const override;
	
	Point getCentroid(size_type index) const override;
	void sortByCentroid(Axis axis) override;

	std::optional<RayHitInfo> traceRay(const Ray& ray) const override;

private:
	TriangleMesh* cloneImpl() override;
	std::pair<IShapeList<RayHitInfo>*, IShapeList<RayHitInfo>*> splitImpl(size_type leftSideElemCount) const override;

	std::shared_ptr<TriangleMeshData> data;
	mutable std::optional<AABB> aabb;
	mutable std::optional<Point> centroid;
	size_type beginIdx;
	size_type endIdx;
};