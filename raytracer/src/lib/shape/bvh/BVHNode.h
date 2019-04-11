#pragma once

#include <array>
#include <variant>
#include <iostream>
#include "shape/Box.h"
#include "math/Axis.h"

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

	std::optional<TRayHitInfo> traceRay(const Ray& ray) const
	{
		//BVHDiag::Levels++;

		if(isLeafNode())
		{
			return leafData().traceRay(ray);
		}
		else
		{
			std::array<float, Arity> aabbHitTs;
			for (int i = 0; i < Arity; i++)
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
							for (int j = i + 1; j < Arity; j++)
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
			}else if(order < 0)
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
			}else
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

private:
	AABB boundingBox;
	std::variant<BVHSubnodeArray, TContentPtr> data;
	Axis childOrder;
};