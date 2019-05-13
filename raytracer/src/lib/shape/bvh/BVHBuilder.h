#pragma once

#include <iostream>
#include <future>
#include <mutex>
#include <tbb/task.h>

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

	std::variant<NodePtr, IncompleteNode> buildNode(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);

	NodePtr buildTree(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);
	std::pair<NodePtr, size_t> buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);

	static void logLeafNodeSizes(const Node& bvh, Statistics::Collector* stats);

	inline static std::mutex exec_mutex;
};

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
	NodePtr* outputTreePtr;
	size_t* treeSizePtr;

	NodeBuildingTask(BVHBuilder<TRayHitInfo>& builder, IShapeList<TRayHitInfo>& shapes, Axis presortedAxis, /* OUTPUT */ NodePtr* outputTree, /* OUTPUT */ size_t* treeSize)
		: builder(std::ref(builder)), shapes(std::ref(shapes)), presortedAxis(presortedAxis), outputTreePtr(outputTree), treeSizePtr(treeSize)
	{ }

	task* execute() override
	{
		auto& tree = *outputTreePtr;
		auto& treeSize = *treeSizePtr;

		auto result = builder.get().buildNode(shapes.get(), presortedAxis);
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
			NodeBuildingTask& a = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.leftSubList, incompleteNode.presortedAxis, &subNodeA, &subNodeSizeA);
			NodeBuildingTask& b = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.rightSubList, incompleteNode.presortedAxis, &subNodeB, &subNodeSizeB);

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
	TRayHitInfo>::buildNode(IShapeList<TRayHitInfo> & shapes, Axis presortedAxis)
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
			shapes.sortByCentroid(axis);
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
	shapes.sortByCentroid(currentSorting);
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
	shapes.sortByCentroid(currentSorting);

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
			shapes.sortByCentroid(bestAxis);
		}

		// Split objects in [0 .. bestSplit) and [bestSplit .. shapeCount)
		auto [listA, listB] = shapes.split(bestSplit);
		return IncompleteNode{ std::make_unique<Node>(totalAABB, currentSorting), std::move(listA), std::move(listB), currentSorting };
	}
}

template <typename TRayHitInfo>
std::unique_ptr<typename BVHBuilder<TRayHitInfo>::Node> BVHBuilder<TRayHitInfo>::buildTree(IShapeList<TRayHitInfo> & shapes, Axis presortedAxis)
{
	auto result = buildNode(shapes, presortedAxis);
	if (std::holds_alternative<NodePtr>(result))
	{
		NodePtr node(std::move(std::get<NodePtr>(result)));
		return node;
	}
	else
	{
		auto& incompleteNode = std::get<IncompleteNode>(result);
		NodePtr node(std::move(incompleteNode.node));
		node->setChild(0, this->buildTree(*incompleteNode.leftSubList, incompleteNode.presortedAxis));
		node->setChild(1, this->buildTree(*incompleteNode.rightSubList, incompleteNode.presortedAxis));
		return node;
	}
}

template <typename TRayHitInfo>
std::pair<std::unique_ptr<typename BVHBuilder<TRayHitInfo>::Node>, size_t> BVHBuilder<TRayHitInfo>::buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis)
{
	NodePtr node;
	size_t size;
	auto& task = *new(tbb::task::allocate_root()) NodeBuildingTask<TRayHitInfo>(*this, shapes, presortedAxis, &node, &size);
	tbb::task::spawn_root_and_wait(task);
	return std::make_pair(std::move(node), size);
}

template<typename TRayHitInfo>
void BVHBuilder<TRayHitInfo>::logLeafNodeSizes(const BVHBuilder::Node &bvh, Statistics::Collector *stats)
{
    if(!bvh.isLeafNode())
    {
        logLeafNodeSizes(bvh.getChild(0), stats);
        logLeafNodeSizes(bvh.getChild(1), stats);
        return;
    }

    stats->log("logLeafNodeSizes", "BVHLeafNodeSize", bvh.leafData().count());
}


template <typename TRayHitInfo>
BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> BVHBuilder<TRayHitInfo>::buildBVH(IShapeList<TRayHitInfo> & shapes, Statistics::Collector* stats)
{
	shapes.sortByCentroid(Axis::x);

	std::lock_guard lock(exec_mutex);

	auto intersectionCost = 1;
	auto traversalCost = 4;
	BVHBuilder builder(intersectionCost, traversalCost);
	auto [rootNode, size] = builder.buildTreeThreaded(shapes, Axis::x);
	if(stats != nullptr)
    {
        logLeafNodeSizes(*rootNode, stats);
    }
	return BVH<ShapeList, TRayHitInfo, 2>(std::move(rootNode), size);
}

