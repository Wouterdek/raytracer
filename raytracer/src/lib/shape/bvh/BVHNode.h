#pragma once

#include <array>
#include <variant>
#include <iostream>
#include <array>
#include <numeric>
#include "shape/AABB.h"
#include "math/Axis.h"

namespace BVHDiag
{
	inline thread_local int MultiRayLevels = 0;
    inline thread_local int SingleRayLevels = 0;
};

template<typename TContent, typename TRayHitInfo, size_t Arity, template<class> class SubNodePointer>
class BVHNode
{
public:
	//using BVHNodePointer = std::unique_ptr<BVHNode<TContent, TRayHitInfo, Arity>>;
	using BVHNodePointer = SubNodePointer<BVHNode<TContent, TRayHitInfo, Arity, SubNodePointer>>;
	using TContentPtr = std::unique_ptr<TContent>;
	using BVHSubnodeArray = std::array<BVHNodePointer, Arity>;

	using size_type = typename BVHSubnodeArray::size_type;

	explicit BVHNode(AABB boundingBox)
		: boundingBox(std::move(boundingBox)), data(BVHSubnodeArray()), childOrder(Axis::x)
	{ }

	BVHNode(AABB boundingBox, Axis sortingAxis)
		: boundingBox(std::move(boundingBox)), data(BVHSubnodeArray()), childOrder(sortingAxis)
	{ }

	BVHNode(AABB boundingBox, std::unique_ptr<TContent>&& leafData)
		: boundingBox(std::move(boundingBox)), data(std::move(leafData)), childOrder(Axis::x)
	{}

	bool isLeafNode() const
	{
		return std::holds_alternative<TContentPtr>(data);
	}

	TContent& leafData()
	{
		return *std::get<TContentPtr>(data);
	}

	const TContent& leafData() const
	{
		return *std::get<TContentPtr>(data);
	}

	TContentPtr& getLeafDataPtr()
	{
		return std::get<TContentPtr>(data);
	}


	BVHNode<TContent, TRayHitInfo, Arity, SubNodePointer>& getChild(size_type i)
	{
		return *std::get<BVHSubnodeArray>(data)[i];
	}

	const BVHNode<TContent, TRayHitInfo, Arity, SubNodePointer>& getChild(size_type i) const
	{
		return *std::get<BVHSubnodeArray>(data)[i];
	}

	BVHNodePointer& getChildPtr(size_type i)
	{
		return std::get<BVHSubnodeArray>(data)[i];
	}

	void setChild(size_type i, BVHNodePointer node)
	{
		std::get<BVHSubnodeArray>(data)[i] = std::move(node);
	}

	Axis getSortedAxis() const
	{
		return this->childOrder;
	}

	void setSortedAxis(Axis axis)
	{
		this->childOrder = axis;
	}

	const AABB& getAABB() const
	{
		return this->boundingBox;
	}

    void traceRays(RBSize_t startIdx, RBSize_t endIdx, RayBundle& rays, RayBundlePermutation& perm,
            HitBundle<TRayHitInfo>& result, std::array<bool, RayBundleSize>& foundBetterHit) const
    {
        //BVHDiag::MultiRayLevels += rays.size();

        if(isLeafNode())
        {
            leafData().traceRays(startIdx, endIdx, rays, perm, result, foundBetterHit);
        }
        else
        {
            // Count number of each ray type (by direction through node)

            std::array<RBSize_t, 3> rayTypeEndOffsets {}; // Count types of ray: [backward, parallel, forward]

            for(RBSize_t rayI = startIdx; rayI < endIdx; ++rayI)
            {
                const auto &ray = rays[rayI];

                auto order = ray.getDirection()[static_cast<int>(this->childOrder)];
                auto sign = (order > 0) - (order < 0);
                rayTypeEndOffsets[sign + 1]++;
            }

            // Turn counts into offsets in rays array.
            rayTypeEndOffsets[0] += startIdx;
            rayTypeEndOffsets[1] += rayTypeEndOffsets[0];
            rayTypeEndOffsets[2] += rayTypeEndOffsets[1];

            {
                // Sort the rays based on which child they will intersect first
                // BACKWARDS | PARALLEL | FORWARD
                RBSize_t i = startIdx;
                for(int typeI = 0; typeI < rayTypeEndOffsets.size(); ++typeI)
                {
                    for(; i < rayTypeEndOffsets[typeI]; ++i)
                    {
                        const auto &ray = rays[i];

                        auto order = ray.getDirection()[static_cast<int>(this->childOrder)];
                        auto sign = (order > 0) - (order < 0);

                        if(sign + 1 != typeI)
                        {
                            for(RBSize_t j = i+1; j < endIdx; ++j)
                            {
                                const auto &rayB = rays[j];

                                auto orderB = rayB.getDirection()[static_cast<int>(this->childOrder)];
                                auto signB = (orderB > 0) - (orderB < 0);

                                if(signB + 1 == typeI)
                                {
                                    std::swap(rays[i], rays[j]);
                                    std::swap(perm[i], perm[j]);
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Test rays against child AABBs
            std::array<std::array<float, Arity>, RayBundleSize> aabbHitTs;
            for(RBSize_t rayI = startIdx; rayI < endIdx; ++rayI)
            {
                const auto &ray = rays[rayI];
                for (int i = 0; i < Arity; ++i)
                {
                    aabbHitTs[rayI][i] = getChild(i).boundingBox.getIntersection(ray);
                }
            }

            auto intersectRaysWithChild = [](RayBundle& rays, RayBundlePermutation& perm, HitBundle<TRayHitInfo>& result,
                    std::array<bool, RayBundleSize>& foundBetterHit,
                    std::array<std::array<float, Arity>, RayBundleSize>& aabbHitTs, RBSize_t raysOfTypeIEndIdx,
                    std::array<bool, RayBundleSize>& rayDone, int childI, const BVHNode& child,
                    RBSize_t intersectingRangeStart, RBSize_t& intersectingRangeEnd
                ){
                // Sort the rays based on whether they intersect the current child AABB
                int rayI = intersectingRangeStart;
                for (; rayI < raysOfTypeIEndIdx; ++rayI)
                {
                    if(rayDone[rayI] || !(aabbHitTs[rayI][childI] >= 0)) // ray done or does not intersect with child
                    {
                        bool foundIntersecting = false;
                        for (int j = rayI+1; j < raysOfTypeIEndIdx; ++j)
                        {
                            if(!rayDone[j] && aabbHitTs[j][childI] >= 0) // ray not done and does intersect with child
                            {
                                std::swap(rays[rayI], rays[j]);
                                std::swap(perm[rayI], perm[j]);
                                foundIntersecting = true;
                                break;
                            }
                        }
                        if(!foundIntersecting)
                        {
                            break;
                        }
                    }
                }
                intersectingRangeEnd = rayI;

                int rangeLength = intersectingRangeEnd - intersectingRangeStart;
                if(rangeLength > 1)
                {
                    child.traceRays(intersectingRangeStart, intersectingRangeEnd, rays, perm, result, foundBetterHit);
                }
                else if(rangeLength == 1)
                {
                    const auto &ray = rays[intersectingRangeStart];
                    auto hit = child.traceRay(ray);
                    if(hit.has_value())
                    {
                        auto& entry = result[perm[intersectingRangeStart]];
                        if(!entry.has_value() || entry->t > hit->t)
                        {
                            entry = hit;
                            foundBetterHit[perm[intersectingRangeStart]] = true;
                        }
                    }
                }
            };

            std::array<bool, RayBundleSize> rayDone {};
            // Backward rays
            {
                int typeI = 0;
                for (int childI = Arity-1; childI >= 0; --childI)
                {

                    RBSize_t intersectingRangeStart = startIdx;
                    RBSize_t intersectingRangeEnd;

                    intersectRaysWithChild(rays, perm, result, foundBetterHit, aabbHitTs, rayTypeEndOffsets[typeI], rayDone, childI, getChild(childI), intersectingRangeStart, intersectingRangeEnd);

                    for (RBSize_t rayI = intersectingRangeStart; rayI < intersectingRangeEnd; ++rayI)
                    {
                        auto& bestHit = result[perm[rayI]];
                        if (bestHit.has_value())
                        {
                            rayDone[rayI] = true;
                            for (int j = childI - 1; j >= 0; --j)
                            {
                                if (aabbHitTs[rayI][j] >= 0 && bestHit->t > aabbHitTs[rayI][j])
                                {
                                    rayDone[rayI] = false;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Forward rays
            {
                int typeI = 2;
                for (int childI = 0; childI < Arity; ++childI)
                {
                    RBSize_t intersectingRangeStart = rayTypeEndOffsets[typeI - 1];
                    RBSize_t intersectingRangeEnd;

                    intersectRaysWithChild(rays, perm, result, foundBetterHit, aabbHitTs, rayTypeEndOffsets[typeI], rayDone, childI, getChild(childI), intersectingRangeStart, intersectingRangeEnd);

                    for (RBSize_t rayI = intersectingRangeStart; rayI < intersectingRangeEnd; ++rayI)
                    {
                        auto& bestHit = result[perm[rayI]];
                        if (bestHit.has_value())
                        {
                            rayDone[rayI] = true;
                            for (int j = childI + 1; j < Arity; ++j)
                            {
                                if (aabbHitTs[rayI][j] >= 0 && bestHit->t > aabbHitTs[rayI][j])
                                {
                                    rayDone[rayI] = false;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            // Parallel rays
            {
                int typeI = 1;
                for (int childI = 0; childI < Arity; ++childI)
                {
                    RBSize_t intersectingRangeStart = rayTypeEndOffsets[typeI - 1];
                    RBSize_t intersectingRangeEnd;

                    intersectRaysWithChild(rays, perm, result, foundBetterHit, aabbHitTs, rayTypeEndOffsets[typeI], rayDone, childI, getChild(childI), intersectingRangeStart, intersectingRangeEnd);
                }
            }
        }
    }

    HitBundle<TRayHitInfo> traceRays(RayBundle& rays) const
    {
        HitBundle<TRayHitInfo> result;
        RayBundlePermutation perm;
        std::iota(perm.begin(), perm.end(), 0);
        std::array<bool, RayBundleSize> foundBetterHit {};
        traceRays(0, RayBundleSize, rays, perm, result, foundBetterHit);
        return result;
    }

	std::optional<TRayHitInfo> traceRay(const Ray& ray) const
	{
		//BVHDiag::SingleRayLevels++;

        if(isLeafNode())
        {
            return leafData().traceRay(ray);
        }
        else
        {
            std::array<float, Arity> aabbHitTs;
            for (int i = 0; i < Arity; ++i)
            {
                aabbHitTs[i] = getChild(i).boundingBox.getIntersection(ray);
            }

            std::optional<TRayHitInfo> bestHit = std::nullopt;
            auto order = ray.getDirection()[static_cast<int>(this->childOrder)];
            if(order > 0)
            {
                for (int i = 0; i < Arity; ++i)
                {
                    if (aabbHitTs[i] >= 0)
                    {
                        auto hit = getChild(i).traceRay(ray);
                        if (hit.has_value() && (!bestHit.has_value() || bestHit->t > hit->t))
                        {
                            bestHit = hit;

                            bool earlyExit = true;
                            for (int j = i + 1; j < Arity; ++j)
                            {
                                if (aabbHitTs[j] >= 0 && hit->t > aabbHitTs[j])
                                {
                                    earlyExit = false;
                                    break;
                                }
                            }
                            if (earlyExit)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if(order < 0)
            {
                for (int i = Arity-1; i >= 0; --i)
                {
                    if (aabbHitTs[i] >= 0)
                    {
                        auto hit = getChild(i).traceRay(ray);
                        if (hit.has_value() && (!bestHit.has_value() || bestHit->t > hit->t))
                        {
                            bestHit = hit;

                            bool earlyExit = true;
                            for (int j = i - 1; j >= 0; --j)
                            {
                                if (aabbHitTs[j] >= 0 && hit->t > aabbHitTs[j])
                                {
                                    earlyExit = false;
                                    break;
                                }
                            }
                            if (earlyExit)
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                for (int i = Arity - 1; i >= 0; --i)
                {
                    if (aabbHitTs[i] >= 0)
                    {
                        auto hit = getChild(i).traceRay(ray);
                        if (hit.has_value() && (!bestHit.has_value() || bestHit->t > hit->t))
                        {
                            bestHit = hit;
                        }
                    }
                }
            }

            return bestHit;
        }
	}

    std::optional<TRayHitInfo> testVisibility(const Ray& ray, float maxT) const
    {
        if(isLeafNode())
        {
            return leafData().testVisibility(ray, maxT);
        }
        else
        {
            std::array<bool, Arity> aabbHit;
            for (int i = 0; i < Arity; ++i)
            {
                float t0, t1;
                aabbHit[i] = getChild(i).boundingBox.getIntersections(ray, t0, t1) && t0 <= maxT;
            }

            auto order = ray.getDirection()[static_cast<int>(this->childOrder)];
            if(order < 0)
            {
                for (int i = Arity-1; i >= 0; --i)
                {
                    if (aabbHit[i])
                    {
                        auto hit = getChild(i).testVisibility(ray, maxT);
                        if (hit.has_value())
                        {
                            return hit;
                        }
                    }
                }
            }
            else
            {
                for (int i = 0; i < Arity; ++i)
                {
                    if (aabbHit[i])
                    {
                        auto hit = getChild(i).testVisibility(ray, maxT);
                        if (hit.has_value())
                        {
                            return hit;
                        }
                    }
                }
            }
        }

        return std::nullopt;
    }

private:
	AABB boundingBox;
	std::variant<BVHSubnodeArray, TContentPtr> data;
	Axis childOrder;
};