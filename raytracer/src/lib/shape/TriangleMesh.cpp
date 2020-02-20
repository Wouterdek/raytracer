#include "TriangleMesh.h"
#include <Eigen/Dense>
#include <numeric>

#define SOASORT_USE_TBB_PARALLEL
#include "utility/soa_sort.h"

#ifndef NO_TBB
#include <tbb/tbb.h>
#endif

#ifdef ENABLE_SIMD
#pragma GCC optimize("O3","unroll-loops","omit-frame-pointer","inline") //Optimization flags
#pragma GCC option("arch=native","tune=native","no-zero-upper") //Enable AVX
#pragma GCC target("avx")  //Enable AVX
#include <x86intrin.h> //AVX/SSE Extensions
#endif

#include "math/Triangle.h"

TriangleMesh::TriangleMesh(
	std::vector<Point> vertices, std::vector<std::array<uint32_t, 3>> vertexIndices, 
	std::vector<Vector3> normals, std::vector<std::array<uint32_t, 3>> normalIndices,
	std::vector<Vector2> texCoords, std::vector<std::array<uint32_t, 3>> texCoordIndices, bool storePermutation)
	: data(std::make_shared<TriangleMeshData>()), beginIdx(0), endIdx(vertexIndices.size())
{
	data->vertices = std::move(vertices);
	data->vertexIndices = std::move(vertexIndices);
	data->normals = std::move(normals);
	data->normalIndices = std::move(normalIndices);
	data->texCoords = std::move(texCoords);
	data->texCoordIndices = std::move(texCoordIndices);
	if(storePermutation){
        data->permutation = std::vector<uint32_t>(data->vertexIndices.size());
        std::iota(data->permutation->begin(), data->permutation->end(), 0);
	}
}

TriangleMesh::TriangleMesh(std::shared_ptr<TriangleMeshData> data, size_type begin, size_type end)
	: data(std::move(data)), beginIdx(begin), endIdx(end)
{
}

TriangleMesh::TriangleMesh(bool storePermutation)
    : data(std::make_shared<TriangleMeshData>()), beginIdx(0), endIdx(0)
{
    if(storePermutation)
    {
        this->data->permutation = std::vector<uint32_t>();
    }
}

TriangleMeshData* TriangleMeshData::cloneImpl() const {
    return new TriangleMeshData(*this);
}

Point TriangleMesh::getCentroid() const
{
    if (!this->centroid.has_value())
	{
        const auto count = this->count();

        if(count == 0){
            throw std::runtime_error("mesh is empty");
        }

		Point newCentroid = this->getCentroid(0) / count;
		for (size_type i = 1; i < count; i++)
		{
			newCentroid += this->getCentroid(i) / count;
		}
		centroid = newCentroid;
	}

	return this->centroid.value();
}

AABB TriangleMesh::getAABB() const
{
	if(!this->aabb.has_value())
	{
		const auto count = this->count();

        if(count == 0){
            throw std::runtime_error("mesh is empty");
        }

        auto newAABB = this->getAABB(0);
		for(size_type i = 1; i < count; i++)
		{
			newAABB = newAABB.merge(this->getAABB(i));
		}
		aabb = newAABB;
	}

	return this->aabb.value();
}

std::optional<RayHitInfo> TriangleMesh::intersect(const Ray& ray) const
{
	std::optional<RayHitInfo> bestHit;

    for(size_type triangleI = this->beginIdx; triangleI < this->endIdx; ++triangleI)
	{
		const auto& indices = data->vertexIndices[triangleI];
		const auto& a = data->vertices[indices[0]];
        const auto& b = data->vertices[indices[1]];
        const auto& c = data->vertices[indices[2]];

        Triangle::TriangleIntersection intersection;
		bool hasIntersection = Triangle::intersect(ray, a, b, c, intersection);
		if(hasIntersection && (!bestHit.has_value() || bestHit->t > intersection.t))
		{
			float alfa = 1.0f - intersection.beta - intersection.gamma;

			const auto& normalIndices = data->normalIndices[triangleI];
            const auto& aNormal = data->normals[normalIndices[0]];
            const auto& bNormal = data->normals[normalIndices[1]];
            const auto& cNormal = data->normals[normalIndices[2]];
			Vector3 normal = (alfa * aNormal) + (intersection.beta * bNormal) + (intersection.gamma * cNormal);

			Vector2 texcoord;
			Vector3 tangent;
			if(!data->texCoordIndices.empty() && !data->texCoords.empty())
			{
				const auto& texCoordIndices = data->texCoordIndices[triangleI];
                const auto& aTexCoord = data->texCoords[texCoordIndices[0]];
                const auto& bTexCoord = data->texCoords[texCoordIndices[1]];
                const auto& cTexCoord = data->texCoords[texCoordIndices[2]];
				texcoord = (alfa * aTexCoord) + (intersection.beta * bTexCoord) + (intersection.gamma * cTexCoord);

                Eigen::Matrix2f uvMat{};
                uvMat.row(0) = bTexCoord - aTexCoord;
                uvMat.row(1) = cTexCoord - aTexCoord;

                Eigen::Matrix<float, 2, 3> deltaPosMat {};
                deltaPosMat.row(0) = b - a;
                deltaPosMat.row(1) = c - a;

                tangent = (uvMat.inverse() * deltaPosMat).row(0);
			}else{
			    texcoord = Vector2(0, 0);
			    tangent = b - a;
			}

			bestHit = RayHitInfo(ray, intersection.t, normal, texcoord, tangent);
            bestHit->triangleIndex = triangleI;
		}
	}

	return bestHit;
}

std::optional<RayHitInfo> TriangleMesh::testVisibility(const Ray& ray, float maxT) const
{
    for(size_type triangleI = this->beginIdx; triangleI < this->endIdx; ++triangleI)
    {
        const auto& indices = data->vertexIndices[triangleI];
        const auto& a = data->vertices[indices[0]];
        const auto& b = data->vertices[indices[1]];
        const auto& c = data->vertices[indices[2]];

        Triangle::TriangleIntersection intersection;
        bool hasIntersection = Triangle::intersect(ray, a, b, c, intersection);
        if(hasIntersection && intersection.t <= maxT)
        {

            RayHitInfo hit(ray, intersection.t, Vector3(0, 0, 0), Vector2(0, 0), Vector3(0, 0, 0));
            hit.triangleIndex = triangleI;
            return hit;
        }
    }

    return std::nullopt;
}

AABB TriangleMesh::getAABB(size_type index) const
{
	const auto& indices = data->vertexIndices[this->beginIdx + index];
    const auto& p1 = data->vertices[indices[0]];
    const auto& p2 = data->vertices[indices[1]];
    const auto& p3 = data->vertices[indices[2]];

	const auto[startX, endX] = std::minmax({ p1.x(), p2.x(), p3.x() });
	const auto[startY, endY] = std::minmax({ p1.y(), p2.y(), p3.y() });
	const auto[startZ, endZ] = std::minmax({ p1.z(), p2.z(), p3.z() });
	const Point newStart(startX - 1E-4, startY - 1E-4, startZ - 1E-4);
	const Point newEnd(endX + 1E-4, endY + 1E-4, endZ + 1E-4);

	return AABB(newStart, newEnd);
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
	const auto& p1 = data->vertices[indices[0]];
	const auto& p2 = data->vertices[indices[1]];
	const auto& p3 = data->vertices[indices[2]];
	return (p1 + p2 + p3)/3;
}

template<bool AllowParallelization>
void TriangleMesh::sortByCentroidImpl(Axis axis)
{
    //soa_sort_no_perm::sort_cmp(data->vertexIndices.begin() + this->beginIdx, data->vertexIndices.begin() + this->endIdx, [axis = axis, this](const auto a, const auto b)
    auto comparator = [axis = axis, this](const auto& a, const auto& b)
    {
        Point c1;
        {
            const auto& p1 = data->vertices[a[0]];
            const auto& p2 = data->vertices[a[1]];
            const auto& p3 = data->vertices[a[2]];
            c1 = (p1 + p2 + p3)/* / 3 */;
        }

        Point c2;
        {
            const auto& p1 = data->vertices[b[0]];
            const auto& p2 = data->vertices[b[1]];
            const auto& p3 = data->vertices[b[2]];
            c2 = (p1 + p2 + p3)/* / 3*/;
        }

        return c1[static_cast<int>(axis)] < c2[static_cast<int>(axis)];
    };

    auto vertBegin = data->vertexIndices.begin() + this->beginIdx;
    auto vertEnd = data->vertexIndices.begin() + this->endIdx;
    auto normBegin = data->normalIndices.begin() + this->beginIdx;

    if(data->permutation.has_value()){
        if (data->texCoordIndices.empty()) {
            soa_sort::sort_cmp<AllowParallelization>(
                    vertBegin, vertEnd,
                    comparator,
                    normBegin, data->permutation->begin() + this->beginIdx);
        } else {
            soa_sort::sort_cmp<AllowParallelization>(
                    vertBegin, vertEnd,
                    comparator,
                    normBegin, data->texCoordIndices.begin() + this->beginIdx,
                    data->permutation->begin() + this->beginIdx);
        }
    }else{
        if (data->texCoordIndices.empty()) {
            soa_sort::sort_cmp<AllowParallelization>(
                    vertBegin, vertEnd,
                    comparator,
                    normBegin);
        } else {
            soa_sort::sort_cmp<AllowParallelization>(
                    vertBegin, vertEnd,
                    comparator,
                    normBegin, data->texCoordIndices.begin() + this->beginIdx);
        }
    }
}

void TriangleMesh::sortByCentroid(Axis axis, bool allowParallelization)
{
	if(allowParallelization) {
	    this->sortByCentroidImpl<true>(axis);
    } else {
        this->sortByCentroidImpl<false>(axis);
	}
}

const TriangleMeshData& TriangleMesh::getData() const
{
	return *this->data;
}

std::optional<RayHitInfo> TriangleMesh::traceRay(const Ray& ray) const
{
	return this->intersect(ray);
}

TriangleMesh* TriangleMesh::cloneImpl() const
{
	return new TriangleMesh(*this);
}

void TriangleMesh::applyTransform(const Transformation& transform) {
    {
        std::vector<bool> transformed(this->data->vertices.size());
        for(auto i = this->beginIdx; i < this->endIdx; ++i)
        {
            for(auto idx : this->data->vertexIndices[i])
            {
                if(!transformed[idx])
                {
                    this->data->vertices[idx] = transform.transform(this->data->vertices[idx]);
                    transformed[idx] = true;
                }
            }
        }
    }

    {
        std::vector<bool> transformed(this->data->normals.size());
        for(auto i = this->beginIdx; i < this->endIdx; ++i)
        {
            for(auto idx : this->data->normalIndices[i])
            {
                if(!transformed[idx])
                {
                    this->data->normals[idx] = transform.transform(this->data->normals[idx]);
                    transformed[idx] = true;
                }
            }
        }
    }
}

std::vector<TriangleMesh> TriangleMesh::appendMeshes(const std::vector<TriangleMesh*>& meshes)
{
    if(this->count() != this->data->vertexIndices.size())
    {
        throw std::runtime_error("Cannot append mesh to partial mesh");
    }

    std::vector<TriangleMesh> resultingSubMeshes(meshes.size());

    // Add entries to permutation vector, if present
    size_t nbAddedTriangles = 0;
    for(const auto* mesh : meshes)
    {
        nbAddedTriangles += mesh->count();
    }
    if(data->permutation.has_value())
    {
        size_t oldTriangleCount = data->permutation->size();
        data->permutation->resize(this->data->permutation->size() + nbAddedTriangles);
        std::iota(data->permutation->begin() + oldTriangleCount, data->permutation->end(), oldTriangleCount);
    }

    // Calculate offsets of entries in mesh data for each new submesh
    std::vector<size_t> triangleOffsets(meshes.size());
    triangleOffsets[0] = this->endIdx;
    std::vector<size_t> vertexOffsets(meshes.size());
    vertexOffsets[0] = this->data->vertices.size();
    std::vector<size_t> texCoordOffsets(meshes.size());
    texCoordOffsets[0] = this->data->texCoords.size();
    std::vector<size_t> indicesOffsets(meshes.size());
    indicesOffsets[0] = this->data->vertexIndices.size();
    for(auto i = 1ul; i < meshes.size(); ++i)
    {
        triangleOffsets[i] = triangleOffsets[i-1] + meshes[i-1]->count();
        vertexOffsets[i] = vertexOffsets[i-1] + meshes[i-1]->data->vertices.size();
        texCoordOffsets[i] = texCoordOffsets[i-1] + std::max(1ul, meshes[i-1]->data->texCoords.size());
        indicesOffsets[i] = indicesOffsets[i-1] + meshes[i-1]->data->vertexIndices.size();
    }

    // Reserve space for merged mesh data
    auto newVerticesCount = this->data->vertices.size();
    auto newNormalsCount = this->data->normals.size();
    auto newTexCoordsCount = this->data->texCoords.size();
    auto newIndicesCount = this->data->vertexIndices.size();
    for(const auto* mesh : meshes)
    {
        newVerticesCount += mesh->data->vertices.size();
        newNormalsCount += mesh->data->normals.size();
        newTexCoordsCount += std::max(1ul, mesh->data->texCoords.size());
        newIndicesCount += mesh->data->vertexIndices.size();
    }
    this->data->vertices.resize(newVerticesCount);
    this->data->normals.resize(newNormalsCount);
    this->data->texCoords.resize(newTexCoordsCount);
    this->data->vertexIndices.resize(newIndicesCount);
    this->data->normalIndices.resize(newIndicesCount);
    this->data->texCoordIndices.resize(newIndicesCount);

    // Do appending
#ifdef NO_TBB
    for(auto i = 0ul; i < meshes.size(); ++i) {
#else
    tbb::parallel_for(0ul, meshes.size(), [this, &meshes, &triangleOffsets, &vertexOffsets, &texCoordOffsets, &indicesOffsets, &resultingSubMeshes](auto i) {
#endif
        const TriangleMesh& mesh = *meshes[i];

        auto triangleOffset = triangleOffsets[i];
        auto verticesOffset = vertexOffsets[i];
        uint32_t texCoordsOffset = texCoordOffsets[i];
        auto indicesOffset = indicesOffsets[i];

        std::copy(mesh.data->vertices.cbegin(), mesh.data->vertices.cend(), this->data->vertices.begin()+verticesOffset);
        std::transform(mesh.data->vertexIndices.cbegin()+mesh.beginIdx, mesh.data->vertexIndices.cbegin()+mesh.endIdx,
                       this->data->vertexIndices.begin() + indicesOffset, [verticesOffset](const auto& idx){
            return std::array<uint32_t, 3>{static_cast<uint32_t>(idx[0] + verticesOffset),
                                           static_cast<uint32_t>(idx[1] + verticesOffset),
                                           static_cast<uint32_t>(idx[2] + verticesOffset)};
        });

        std::copy(mesh.data->normals.cbegin(), mesh.data->normals.cend(), this->data->normals.begin()+verticesOffset);
        std::transform(mesh.data->normalIndices.cbegin()+mesh.beginIdx, mesh.data->normalIndices.cbegin()+mesh.endIdx,
                       this->data->normalIndices.begin() + indicesOffset, [verticesOffset](const auto& idx){
            return std::array<uint32_t, 3>{static_cast<uint32_t>(idx[0] + verticesOffset),
                                           static_cast<uint32_t>(idx[1] + verticesOffset),
                                           static_cast<uint32_t>(idx[2] + verticesOffset)};
        });


        if(mesh.data->texCoords.empty())
        {
            this->data->texCoords[texCoordsOffset] = Vector2(0, 0);
            std::fill(this->data->texCoordIndices.begin()+indicesOffset, this->data->texCoordIndices.begin() + indicesOffset + mesh.count(),
                    std::array<uint32_t, 3>{texCoordsOffset, texCoordsOffset, texCoordsOffset});
        }
        else
        {
            std::copy(mesh.data->texCoords.cbegin(), mesh.data->texCoords.cend(), this->data->texCoords.begin()+texCoordsOffset);
            std::transform(mesh.data->texCoordIndices.cbegin()+mesh.beginIdx, mesh.data->texCoordIndices.cbegin()+mesh.endIdx,
                           this->data->texCoordIndices.begin()+indicesOffset, [texCoordsOffset](const auto& idx){
                return std::array<uint32_t, 3>{static_cast<uint32_t>(idx[0] + texCoordsOffset),
                                               static_cast<uint32_t>(idx[1] + texCoordsOffset),
                                               static_cast<uint32_t>(idx[2] + texCoordsOffset)};
            });
        }

        resultingSubMeshes[i] = TriangleMesh(data, triangleOffset, triangleOffset + mesh.count());

#ifdef NO_TBB
    }
#else
    });
#endif

    // Update object state
    this->aabb = std::nullopt;
    this->centroid = std::nullopt;
    this->endIdx += nbAddedTriangles;

    return resultingSubMeshes;
}
