#pragma once

#include "BVH.h"
#include "shape/list/IShapeList.h"
#include <iostream>
#include <future>
#include <mutex>

#include <tbb/task.h>

template<typename TRayHitInfo>
class NodeBuildingTask;

template<typename TRayHitInfo>
class BVHBuilder
{
public:
	friend class NodeBuildingTask<TRayHitInfo>;

	static BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> buildBVH(IShapeList<TRayHitInfo>& shapes);

private:
	using size_type = typename IShapeList<TRayHitInfo>::size_type;
	using ShapeList = IShapeList<TRayHitInfo>;
	using ShapeListPtr = std::unique_ptr<ShapeList>;
	using Node = BVHNode<ShapeList, TRayHitInfo, 2>;
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

	std::unique_ptr<Node> buildTree(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);
	std::unique_ptr<Node> buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis);

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

	NodeBuildingTask(BVHBuilder<TRayHitInfo>& builder, IShapeList<TRayHitInfo>& shapes, Axis presortedAxis, /* OUTPUT PARAMETER */ NodePtr* outputTree)
		: builder(std::ref(builder)), shapes(std::ref(shapes)), presortedAxis(presortedAxis), outputTreePtr(outputTree)
	{ }

	task* execute() override
	{
		auto& tree = *outputTreePtr;

		auto result = builder.get().buildNode(shapes.get(), presortedAxis);
		if (std::holds_alternative<NodePtr>(result))
		{
			tree = std::move(std::get<NodePtr>(result));
		}
		else
		{
			auto& incompleteNode = std::get<BVHBuilder<TRayHitInfo>::IncompleteNode>(result);

			NodePtr subNodeA;
			NodePtr subNodeB;
			NodeBuildingTask& a = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.leftSubList, incompleteNode.presortedAxis, &subNodeA);
			NodeBuildingTask& b = *new(allocate_child()) NodeBuildingTask(builder, *incompleteNode.rightSubList, incompleteNode.presortedAxis, &subNodeB);

			set_ref_count(3);
			spawn(b);
			spawn_and_wait_for_all(a);

			tree = std::move(incompleteNode.node);
			tree->setChild(0, std::move(subNodeA));
			tree->setChild(1, std::move(subNodeB));
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
		return IncompleteNode{ std::make_unique<Node>(totalAABB), std::move(listA), std::move(listB), currentSorting };
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
std::unique_ptr<typename BVHBuilder<TRayHitInfo>::Node> BVHBuilder<TRayHitInfo>::buildTreeThreaded(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis)
{
	NodePtr node;
	auto& task = *new(tbb::task::allocate_root()) NodeBuildingTask<TRayHitInfo>(*this, shapes, presortedAxis, &node);
	tbb::task::spawn_root_and_wait(task);
	return node;
}

template <typename TRayHitInfo>
BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> BVHBuilder<TRayHitInfo>::buildBVH(IShapeList<TRayHitInfo> & shapes)
{
	shapes.sortByCentroid(Axis::x);

	std::lock_guard lock(exec_mutex);

	BVHBuilder builder(1, 5);//TODO: intersect cost
	auto rootNode = builder.buildTreeThreaded(shapes, Axis::x);
	return BVH<ShapeList, TRayHitInfo, 2>(std::move(rootNode));
}