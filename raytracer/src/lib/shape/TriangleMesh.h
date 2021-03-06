#pragma once

#include "IShape.h"
#include "math/Vector3.h"
#include "shape/list/IShapeList.h"
#include "math/Vector2.h"

class TriangleMeshData : public ICloneable<TriangleMeshData>
{
public:
	std::vector<Point> vertices;
	std::vector<std::array<uint32_t, 3>> vertexIndices;
	std::vector<Vector3> normals;
	std::vector<std::array<uint32_t, 3>> normalIndices;
	std::vector<Vector2> texCoords;
	std::vector<std::array<uint32_t, 3>> texCoordIndices;
    std::optional<std::vector<uint32_t>> permutation;

private:
    TriangleMeshData* cloneImpl() const override;
};

class TriangleMesh final: public IShape, public IShapeList<RayHitInfo>
{
public:
	TriangleMesh(std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices,
		std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices,
		std::vector<Vector2> texCoords, std::vector<std::array<uint32_t, 3>> texCoordIndices, bool storePermutation = false);
	TriangleMesh(std::shared_ptr<TriangleMeshData> data, size_type begin, size_type end);
    explicit TriangleMesh(bool storePermutation = false);

	Point getCentroid() const override;
	AABB getAABB() const override;
	std::optional<RayHitInfo> intersect(const Ray& ray) const override;

	AABB getAABB(size_type index) const override;
	size_type count() const override;
    size_type getBeginIndex() const
    {
        return this->beginIdx;
    }
    size_type getEndIndex() const
    {
        return this->endIdx;
    }
	
	Point getCentroid(size_type index) const override;
	void sortByCentroid(Axis axis, bool allowParallelization) override;

	const TriangleMeshData& getData() const;

	std::optional<RayHitInfo> traceRay(const Ray& ray) const override;

    std::optional<RayHitInfo> testVisibility(const Ray &ray, float maxT) const override;

    void applyTransform(const Transformation& transform);
    std::vector<TriangleMesh> appendMeshes(const std::vector<TriangleMesh*>& meshes);

private:
	TriangleMesh* cloneImpl() const override;
	std::pair<IShapeList<RayHitInfo>*, IShapeList<RayHitInfo>*> splitImpl(size_type leftSideElemCount) const override;
	template<bool AllowParallelization>
    void sortByCentroidImpl(Axis axis);

	std::shared_ptr<TriangleMeshData> data;
	mutable std::optional<AABB> aabb;
	mutable std::optional<Point> centroid;
	size_type beginIdx;
	size_type endIdx;
};