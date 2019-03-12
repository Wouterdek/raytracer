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

template<typename TContent, typename TRayHitInfo, size_t Arity>
class BVHNode
{
public:
	using BVHNodePointer = std::unique_ptr<BVHNode<TContent, TRayHitInfo, Arity>>;
	using TContentPtr = std::unique_ptr<TContent>;
	using BVHSubnodeArray = std::array<BVHNodePointer, Arity>;

	using size_type = typename BVHSubnodeArray::size_type;

	explicit BVHNode(Box boundingBox)
		: boundingBox(std::move(boundingBox)), data(BVHSubnodeArray())
	{ }

	BVHNode(Box boundingBox, std::unique_ptr<TContent>&& leafData)
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
		if(isLeafNode())
		{
			return leafData().traceRay(ray);
		}
		else
		{
			std::optional<RayHitInfo> bestHit;
			int bestHitI = 0;
			for(int i = 0; i < Arity; i++)
			{
				const auto& subNode = getChild(i);
				auto hit = subNode.boundingBox.intersect(ray);
				if(hit.has_value() && (!bestHit.has_value() || bestHit->t < hit->t))
				{
					bestHit = hit;
					bestHitI = i;
				}
			}

			if(!bestHit.has_value())
			{
				return std::nullopt;
			}

			return getChild(bestHitI).traceRay(ray);
		}
	}

private:
	Box boundingBox;
	std::variant<BVHSubnodeArray, TContentPtr> data;
};