#pragma once

#include "IShape.h"
#include "math/Vector3.h"
#include "shape/list/IShapeList.h"

struct TriangleMeshData
{
	std::vector<Point> vertices;
	std::vector<std::array<uint32_t, 3>> vertexIndices;
	std::vector<Vector3> normals;
	std::vector<std::array<uint32_t, 3>> normalIndices;
};

class TriangleMesh : public IShape, public IShapeList<RayHitInfo>
{
public:
	TriangleMesh(std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices, std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices);
	TriangleMesh(std::shared_ptr<TriangleMeshData> data, size_type begin, size_type end);

	Point getCentroid() const override;
	Box getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const override;

	Box getAABB(size_type index) const override;
	size_type count() const override;
	
	Point getCentroid(size_type index) const;
	void sortByCentroid(Axis axis) override;

	std::optional<RayHitInfo> traceRay(const Ray& ray) const override;

private:
	TriangleMesh* cloneImpl() override;
	std::pair<IShapeList<RayHitInfo>*, IShapeList<RayHitInfo>*> splitImpl(size_type leftSideElemCount) const override;

	std::shared_ptr<TriangleMeshData> data;
	mutable std::optional<Box> aabb;
	mutable std::optional<Point> centroid;
	size_type beginIdx;
	size_type endIdx;
};


class A
{
	virtual A* testX();
	virtual std::pair<A*, int> testY();
	virtual std::tuple<A*, int> testZ();
};