#pragma once

#include <memory>
#include "BVHNode.h"
#include <queue>
#include "utility/raw_pointer.h"
#include "utility/unique_ptr_template.h"

template<typename TContent, typename TRayHitInfo, size_t Arity>
class BVH
{
public:
	using LinkedNode = BVHNode<TContent, TRayHitInfo, Arity, unique_pointer>;
	using PackedNode = BVHNode<TContent, TRayHitInfo, Arity, raw_pointer>;

	explicit BVH(std::unique_ptr<LinkedNode> rootNode, size_t treeSize)
		: nodes(std::move(rootNode)), treeSize(treeSize)
	{ }

	std::optional<TRayHitInfo> traceRay(const Ray& ray) const
	{
		if(std::holds_alternative<std::unique_ptr<LinkedNode>>(nodes))
		{
			return std::get<std::unique_ptr<LinkedNode>>(nodes)->traceRay(ray);
		}else
		{
			return std::get<std::vector<PackedNode>>(nodes)[0].traceRay(ray);
		}
	}

	size_t getSize() const
	{
		return treeSize;
	}

	void pack()
	{
		if(!std::holds_alternative<std::unique_ptr<LinkedNode>>(nodes))
		{
			throw std::runtime_error("BVH is already packed or has no nodes");
		}
		std::unique_ptr<LinkedNode> root = std::move(std::get<std::unique_ptr<LinkedNode>>(nodes));

		std::vector<PackedNode> packedNodes;
		packedNodes.reserve(treeSize);

		std::queue<std::tuple<std::unique_ptr<LinkedNode>, PackedNode*, int>> remainingNodes;
		remainingNodes.push(std::make_tuple(std::move(root), nullptr, 0));

		while(!remainingNodes.empty())
		{
			auto& cur = remainingNodes.front();
			std::unique_ptr<LinkedNode> node = std::move(std::get<0>(cur));
			PackedNode* parentNode = std::get<1>(cur);
			int childI = std::get<2>(cur);
			remainingNodes.pop();

			if(node->isLeafNode())
			{
				packedNodes.emplace_back(node->getAABB(), std::move(node->getLeafDataPtr()));
			}
			else
			{
				packedNodes.emplace_back(node->getAABB(), node->getSortedAxis());
				for(int i = 0; i < Arity; i++)
				{
					remainingNodes.push(std::make_tuple(std::move(node->getChildPtr(i)), &packedNodes.back(), i));
				}
			}
			PackedNode& curNode = packedNodes.back();

			if(parentNode != nullptr)
			{
				parentNode->setChild(childI, raw_pointer(&curNode));
			}
		}

		nodes = std::move(packedNodes);
	}

private:
	std::variant<
		std::unique_ptr<LinkedNode>,
		std::vector<PackedNode>
	> nodes;
	size_t treeSize;
};
