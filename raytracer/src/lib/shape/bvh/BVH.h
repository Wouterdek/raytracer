#pragma once

#include <memory>
#include "BVHNode.h"

template<typename TContent, typename TRayHitInfo, size_t Arity>
class BVH
{
public:
	using Node = BVHNode<TContent, TRayHitInfo, Arity>;

	explicit BVH(std::unique_ptr<Node> rootNode)
		: root(std::move(rootNode))
	{ }

	std::optional<TRayHitInfo> traceRay(const Ray& ray)
	{
		return root->traceRay(ray);
	}

private:
	std::unique_ptr<Node> root;
};