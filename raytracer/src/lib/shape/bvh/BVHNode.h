#pragma once

#include <array>
#include <variant>
#include "shape/Box.h"

/*template<typename TContent, typename TRayHitInfo>
struct HasTraceRayMethod
{
private:
	template<typename U>
	static void test_sig(
		std::optional<TRayHitInfo> (U::*)(const Ray&)
	);

	using Yes = char;
	using No = int;

	template<typename UContent, typename URayHitInfo>
	static decltype(test_sig(&TContent::traceRay), Yes()) test(int);
	template<typename>
	static No test(...);
public:
	HasTraceRayMethod() = delete;
	enum { value = sizeof(test<TContent, TRayHitInfo>(0)) == sizeof(Yes) };
};*/

namespace BVHDiag
{
	inline thread_local int Levels = 0;
};

template<typename TContent, typename TRayHitInfo, size_t Arity>
class BVHNode
{
public:
	using BVHNodePointer = std::unique_ptr<BVHNode<TContent, TRayHitInfo, Arity>>;
	using TContentPtr = std::unique_ptr<TContent>;
	using BVHSubnodeArray = std::array<BVHNodePointer, Arity>;

	using size_type = typename BVHSubnodeArray::size_type;

	explicit BVHNode(AABB boundingBox)
		: boundingBox(std::move(boundingBox)), data(BVHSubnodeArray())
	{ }

	BVHNode(AABB boundingBox, std::unique_ptr<TContent>&& leafData)
		: boundingBox(std::move(boundingBox)), data(std::move(leafData))
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


	BVHNode<TContent, TRayHitInfo, Arity>& getChild(size_type i)
	{
		return *std::get<BVHSubnodeArray>(data)[i];
	}

	const BVHNode<TContent, TRayHitInfo, Arity>& getChild(size_type i) const
	{
		return *std::get<BVHSubnodeArray>(data)[i];
	}

	void setChild(size_type i, BVHNodePointer node)
	{
		std::get<BVHSubnodeArray>(data)[i] = std::move(node);
	}

	std::optional<TRayHitInfo> traceRay(const Ray& ray) const
	{
		//BVHDiag::Levels++;

		if(isLeafNode())
		{
			return leafData().traceRay(ray);
		}
		else
		{
			/*
			 * OPTIMIZATION OPPORTUNITY
			 * if node subnodes are ordered, we can eliminate subnode checks as follows:
			 * 
			 * node is ordered by axis i
			 * order = ray.dir[i]
			 * if order > 0:
			 *   bestT = infinity
			 *   j from 0 to Arity:
			 *     intersect subnode j
			 *     if intersection != null && t < bestT:
			 *       if intersection[i] < subnode[j+1].aabb_start[i]:
			 *         return intersection
			 *       else
			 *         bestT = t
			 *   return bestT
			 * else if order < 0
			 *   do same, but j from Arity to 0 (reverse)
			 * else
			 *   check all nodes for intersection, return smallest T
			 */

			std::optional<TRayHitInfo> bestHit;
			for(int i = 0; i < Arity; i++)
			{
				const auto& subNode = getChild(i);
				if(subNode.boundingBox.intersects(ray))
				{
					auto hit = getChild(i).traceRay(ray);
					if(hit.has_value() && (!bestHit.has_value() || bestHit->t > hit->t))
					{
						bestHit = hit;
					}
				}
			}

			if(!bestHit.has_value())
			{
				return std::nullopt;
			}

			return bestHit;
		}
	}

private:
	AABB boundingBox;
	std::variant<BVHSubnodeArray, TContentPtr> data;
};