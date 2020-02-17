#pragma once

#include <iostream>
#include <future>
#include <mutex>

#ifndef NO_TBB
#include <tbb/task.h>
#endif

#include "BVH.h"
#include "shape/list/IShapeList.h"
#include "utility/unique_ptr_template.h"
#include "utility/StatCollector.h"

template<typename TRayHitInfo>
class NodeBuildingTask;

template<typename TRayHitInfo>
class BVHBuilder
{
public:
	friend class NodeBuildingTask<TRayHitInfo>;

	static BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> buildBVH(IShapeList<TRayHitInfo>& shapes, Statistics::Collector* stats = nullptr);

private:
	using size_type = typename IShapeList<TRayHitInfo>::size_type;
	using ShapeList = IShapeList<TRayHitInfo>;
	using ShapeListPtr = std::unique_ptr<ShapeList>;
	using Node = BVHNode<ShapeList, TRayHitInfo, 2, unique_pointer>;
	using NodePtr = std::unique_ptr<Node>;

	struct IncompleteNode
	{
		NodePtr node;
		ShapeListPtr leftSubList;
		ShapeListPtr rightSubList;
		Axis presortedAxis;
	};

	BVHBuilder(double costPerIntersection, double costPerTraversal);

	const double costPerIntersection;
	const double costPerTraversal;

	double calculateSAH(size_type s1Count, double s1AABBArea, size_type s2Count, double s2AABBArea, double totalAABBArea) const;

	std::variant<NodePtr, IncompleteNode> buildNode(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis, bool allowParallelization);

    std::pair<NodePtr, size_t> buildTree(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);
#ifndef NO_TBB
	std::pair<NodePtr, size_t> buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);
#endif

	struct LeafNodeSizeStats {
        unsigned long totalLeafNodeSize;
        unsigned long totalLeafNodeCount;
        unsigned int smallestLeafNode;
        unsigned int largestLeafNode;
	};
    static LeafNodeSizeStats gatherLeafNodeSizeStats(const BVHBuilder::Node &bvh);
	static void logLeafNodeSizes(const Node& bvh, Statistics::Collector* stats);

	inline static std::mutex exec_mutex;
};

#ifndef NO_TBB
template<typename TRayHitInfo>
class NodeBuildingTask : public tbb::task
{
public:
	using ShapeList = typename BVHBuilder<TRayHitInfo>::ShapeList;
	using ShapeListPtr = typename BVHBuilder<TRayHitInfo>::ShapeListPtr;
	using Node = typename BVHBuilder<TRayHitInfo>::Node;
	using NodePtr = typename BVHBuilder<TRayHitInfo>::NodePtr;

	std::reference_wrapper<BVHBuilder<TRayHitInfo>> builder;
	std::reference_wrapper<IShapeList<TRayHitInfo>> shapes;
	Axis presortedAxis;
	unsigned int depth;
	NodePtr* outputTreePtr;
	size_t* treeSizePtr;

	NodeBuildingTask(BVHBuilder<TRayHitInfo>& builder, IShapeList<TRayHitInfo>& shapes, Axis presortedAxis, unsigned int depth, /* OUTPUT */ NodePtr* outputTree, /* OUTPUT */ size_t* treeSize)
		: builder(std::ref(builder)), shapes(std::ref(shapes)), presortedAxis(presortedAxis), depth(depth), outputTreePtr(outputTree), treeSizePtr(treeSize)
	{ }

	task* execute() override
	{
		auto& tree = *outputTreePtr;
		auto& treeSize = *treeSizePtr;

        bool allowParallelization = depth < 2;
		auto result = builder.get().buildNode(shapes.get(), presortedAxis, allowParallelization);
		if (std::holds_alternative<NodePtr>(result))
		{
			tree = std::move(std::get<NodePtr>(result));
			treeSize = 1;
		}
		else
		{
			auto& incompleteNode = std::get<typename BVHBuilder<TRayHitInfo>::IncompleteNode>(result);

			NodePtr subNodeA;
			size_t subNodeSizeA;
			NodePtr subNodeB;
			size_t subNodeSizeB;
			NodeBuildingTask& a = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.leftSubList, incompleteNode.presortedAxis, depth+1, &subNodeA, &subNodeSizeA);
			NodeBuildingTask& b = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.rightSubList, incompleteNode.presortedAxis, depth+1, &subNodeB, &subNodeSizeB);

			set_ref_count(3);
			spawn(b);
			spawn_and_wait_for_all(a);

			tree = std::move(incompleteNode.node);
			tree->setChild(0, std::move(subNodeA));
			tree->setChild(1, std::move(subNodeB));
			treeSize = subNodeSizeA + subNodeSizeB + 1;
		}
		return nullptr;
	}
};
#endif

/***********************************/
/**** BVHBuilder implementation ****/
/***********************************/

template <typename TRayHitInfo>
BVHBuilder<TRayHitInfo>::BVHBuilder(double costPerIntersection, double costPerTraversal)
	: costPerIntersection(costPerIntersection), costPerTraversal(costPerTraversal)
{}

template <typename TRayHitInfo>
double BVHBuilder<TRayHitInfo>::calculateSAH(size_type s1Count, double s1AABBArea, size_type s2Count, double s2AABBArea, double totalAABBArea) const
{
	return costPerTraversal + ((s1AABBArea / totalAABBArea) * s1Count * costPerIntersection) + ((s2AABBArea / totalAABBArea) * s2Count * costPerIntersection);
}

inline thread_local std::vector<float> leftArea; // TODO: is this cleaned up?

template <typename TRayHitInfo>
std::variant<typename BVHBuilder<TRayHitInfo>::NodePtr, typename BVHBuilder<TRayHitInfo>::IncompleteNode> BVHBuilder<
	TRayHitInfo>::buildNode(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis, bool allowParallelization)
{
	assert(shapes.count() > 0);

	if (shapes.count() == 1)
	{
		// Create leaf node
		return std::make_unique<Node>(shapes.getAABB(0), shapes.clone());
	}

	if (leftArea.size() < shapes.count())
	{
		leftArea.resize(shapes.count());
	}

	// Choose optimal split point or choose leaf mode
	const size_t shapeCount = shapes.count();

	double bestCost = shapes.count() * costPerIntersection;

	bool leavesAreCheapest = true;
	size_type bestSplit;
	Axis bestAxis;
	Axis currentSorting = presortedAxis;
	AABB totalAABB;

#define SAH
#ifdef SAH
	for (auto axis : getAxesStartingWith(presortedAxis))
	{
		if (currentSorting != axis)
		{
			shapes.sortByCentroid(axis, allowParallelization);
			currentSorting = axis;
		}

		// Calculate the cumulative AABB surface area of the shapes, starting from the left
		AABB boxAcc = shapes.getAABB(0);
		leftArea[0] = static_cast<float>(boxAcc.getSurfaceArea());
		for (size_type i = 1; i < shapeCount; i++)
		{
			boxAcc = boxAcc.merge(shapes.getAABB(i));
			leftArea[i] = static_cast<float>(boxAcc.getSurfaceArea());
		}
		totalAABB = boxAcc;
		const double totalAABBSurf = leftArea[shapeCount - 1]; // last element is surface of AABB with all elements on the left = total AABB

		// Calculate the cumulative surface area of the shapes, starting from the right.
		// Ignore the cases where either side is empty.
		// Use these values to calculate surface area heuristic and find optimal splitting point.
		boxAcc = shapes.getAABB(shapeCount - 1);
		for (size_type leftSideShapeCount = shapeCount - 1; leftSideShapeCount > 0; leftSideShapeCount--)
		{
			const double curCost = calculateSAH(leftSideShapeCount, leftArea[leftSideShapeCount], shapeCount - leftSideShapeCount, boxAcc.getSurfaceArea(), totalAABBSurf);
			if (curCost < bestCost)
			{
				leavesAreCheapest = false;
				bestSplit = leftSideShapeCount;
				bestAxis = axis;
				bestCost = curCost;
			}

			boxAcc = boxAcc.merge(shapes.getAABB(leftSideShapeCount - 1));
		}
	}
#endif

#ifdef OMS
	currentSorting = static_cast<Axis>( (static_cast<int>(presortedAxis) + 1) % 3 );
	shapes.sortByCentroid(currentSorting, allowParallelization);
	leavesAreCheapest = shapeCount <= 10;
	bestSplit = shapeCount / 2;
	bestAxis = currentSorting;
	bestCost = 1;

	AABB boxAcc = shapes.getAABB(0);
	leftArea[0] = static_cast<float>(boxAcc.getSurfaceArea());
	for (size_type i = 1; i < shapeCount; i++)
	{
		boxAcc = boxAcc.merge(shapes.getAABB(i));
		leftArea[i] = static_cast<float>(boxAcc.getSurfaceArea());
	}
	totalAABB = boxAcc;
#endif

#ifdef SMS
	AABB boxAcc = shapes.getAABB(0);
	leftArea[0] = static_cast<float>(boxAcc.getSurfaceArea());
	for (size_type i = 1; i < shapeCount; i++)
	{
		boxAcc = boxAcc.merge(shapes.getAABB(i));
		leftArea[i] = static_cast<float>(boxAcc.getSurfaceArea());
	}
	totalAABB = boxAcc;

	auto currentSortingI = (static_cast<int>(presortedAxis) + 1) % 3;
	currentSorting = static_cast<Axis>(currentSortingI);
	auto center = totalAABB.getStart() + ((totalAABB.getEnd() - totalAABB.getStart())/2);
	auto middle = center[currentSortingI];
	shapes.sortByCentroid(currentSorting, allowParallelization);

	bestSplit = 1;
	while(bestSplit < shapeCount-1 && shapes.getCentroid(bestSplit)[currentSortingI] < middle)
	{
		bestSplit++;
	}
	leavesAreCheapest = shapeCount <= 10;
	bestAxis = currentSorting;
	bestCost = 1;

	
#endif

	if (leavesAreCheapest)
	{
		// Create leaf node
		return std::make_unique<Node>(totalAABB, shapes.clone());
	}
	else
	{
		if (currentSorting != bestAxis)
		{
			shapes.sortByCentroid(bestAxis, allowParallelization);
		}

		// Split objects in [0 .. bestSplit) and [bestSplit .. shapeCount)
		auto [listA, listB] = shapes.split(bestSplit);
		return IncompleteNode{ std::make_unique<Node>(totalAABB, currentSorting), std::move(listA), std::move(listB), currentSorting };
	}
}

template <typename TRayHitInfo>
std::pair<std::unique_ptr<typename BVHBuilder<TRayHitInfo>::Node>, size_t> BVHBuilder<TRayHitInfo>::buildTree(IShapeList<TRayHitInfo> & shapes, Axis presortedAxis)
{
	auto result = buildNode(shapes, presortedAxis, false);
	if (std::holds_alternative<NodePtr>(result))
	{
		return std::make_pair(std::move(std::get<NodePtr>(result)), 1);
	}
	else
	{
		auto& incompleteNode = std::get<IncompleteNode>(result);
		NodePtr node(std::move(incompleteNode.node));
		auto [childA, childASize] = this->buildTree(*incompleteNode.leftSubList, incompleteNode.presortedAxis);
		node->setChild(0, std::move(childA));
        auto [childB, childBSize] = this->buildTree(*incompleteNode.rightSubList, incompleteNode.presortedAxis);
        node->setChild(1, std::move(childB));
		return std::make_pair(std::move(node), childASize + childBSize);
	}
}

#ifndef NO_TBB
template <typename TRayHitInfo>
std::pair<std::unique_ptr<typename BVHBuilder<TRayHitInfo>::Node>, size_t> BVHBuilder<TRayHitInfo>::buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis)
{
	NodePtr node;
	size_t size;
	auto& task = *new(tbb::task::allocate_root()) NodeBuildingTask<TRayHitInfo>(*this, shapes, presortedAxis, 0, &node, &size);
	tbb::task::spawn_root_and_wait(task);
	return std::make_pair(std::move(node), size);
}
#endif

template<typename TRayHitInfo>
typename BVHBuilder<TRayHitInfo>::LeafNodeSizeStats BVHBuilder<TRayHitInfo>::gatherLeafNodeSizeStats(const BVHBuilder::Node &bvh)
{
    BVHBuilder<TRayHitInfo>::LeafNodeSizeStats result;
    if(bvh.isLeafNode())
    {
        result.totalLeafNodeSize = result.smallestLeafNode = result.largestLeafNode = bvh.leafData().count();
        result.totalLeafNodeCount = 1;
    }
    else
    {
        auto info1 = gatherLeafNodeSizeStats(bvh.getChild(0));
        auto info2 = gatherLeafNodeSizeStats(bvh.getChild(1));
        result.totalLeafNodeSize = info1.totalLeafNodeSize + info2.totalLeafNodeSize;
        result.largestLeafNode = std::max(info1.largestLeafNode, info2.largestLeafNode);
        result.smallestLeafNode = std::min(info1.smallestLeafNode, info2.smallestLeafNode);
    }
    return result;
}

template<typename TRayHitInfo>
void BVHBuilder<TRayHitInfo>::logLeafNodeSizes(const BVHBuilder::Node &bvh, Statistics::Collector *stats)
{
    auto info = gatherLeafNodeSizeStats(bvh);
    stats->log("logLeafNodeSizeInfo", "BVHAvgLeafNodeSize", info.totalLeafNodeSize / info.totalLeafNodeCount);
    stats->log("logLeafNodeSizeInfo", "BVHLargestLeafNodeSize", static_cast<unsigned long>(info.largestLeafNode));
    stats->log("logLeafNodeSizeInfo", "BVHSmallestLeafNodeSize", static_cast<unsigned long>(info.smallestLeafNode));
    stats->log("logLeafNodeSizeInfo", "BVHLeafNodeCount", info.totalLeafNodeCount);
}


template <typename TRayHitInfo>
BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> BVHBuilder<TRayHitInfo>::buildBVH(IShapeList<TRayHitInfo> & shapes, Statistics::Collector* stats)
{
	shapes.sortByCentroid(Axis::x, true);

	std::lock_guard lock(exec_mutex);

	auto intersectionCost = 1;
	auto traversalCost = 4;
	BVHBuilder builder(intersectionCost, traversalCost);
#ifdef NO_TBB
	auto [rootNode, size] = builder.buildTree(shapes, Axis::x);
#else
    auto [rootNode, size] = builder.buildTreeThreaded(shapes, Axis::x);
#endif
	if(stats != nullptr)
    {
        logLeafNodeSizes(*rootNode, stats);
    }

	return BVH<ShapeList, TRayHitInfo, 2>(std::move(rootNode), size);
}

