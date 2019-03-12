#pragma once

#include "BVH.h"
#include "shape/list/IShapeList.h"

template<typename TRayHitInfo>
class BVHBuilder
{
private:
	using size_type = typename IShapeList<TRayHitInfo>::size_type;
	using ShapeList = IShapeList<TRayHitInfo>;
	using ShapeListPtr = std::unique_ptr<ShapeList>;

	BVHBuilder(size_type shape_count, double costPerIntersection, double costPerTraversal)
		: costPerIntersection(costPerIntersection), costPerTraversal(costPerTraversal), leftArea(shape_count) {};

	const double costPerIntersection;
	const double costPerTraversal;
	std::vector<float> leftArea;

	double calculateSAH(size_type s1Count, double s1AABBArea, size_type s2Count, double s2AABBArea, double totalAABBArea) const
	{
		return costPerTraversal + ((s1AABBArea / totalAABBArea) * s1Count * costPerIntersection) + ((s2AABBArea / totalAABBArea) * s2Count * costPerIntersection);
	}

	std::unique_ptr<BVHNode<ShapeList, TRayHitInfo, 2>> buildNode(IShapeList<TRayHitInfo>& shapes, Axis presortedAxis)
	{
		assert(shapes.count() > 0);

		if (shapes.count() == 1)
		{
			// Create leaf node
			return std::make_unique<BVHNode<ShapeList, TRayHitInfo, 2>>(shapes.getAABB(0), shapes.clone());
		}

		// Choose optimal split point or choose leaf mode
		const size_t shapeCount = shapes.count();
		
		double bestCost = shapes.count() * costPerIntersection;

		bool leavesAreCheapest = true;
		size_type bestSplit;
		Axis bestAxis;
		Axis currentSorting = presortedAxis;
		Box totalAABB(Point(0, 0, 0), Point(0, 0, 0));
		
		for (auto axis : getAxesStartingWith(presortedAxis))
		{
			if(currentSorting != axis)
			{
				shapes.sortByCentroid(axis);
				currentSorting = axis;
			}

			// Calculate the cumulative AABB surface area of the shapes, starting from the left
			Box boxAcc = shapes.getAABB(0);
			leftArea[0] = boxAcc.getSurfaceArea();
			for (size_type i = 1; i < shapeCount; i++)
			{
				boxAcc = boxAcc.merge(shapes.getAABB(i));
				leftArea[i] = boxAcc.getSurfaceArea();
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
			return std::make_unique<BVHNode<ShapeList, TRayHitInfo, 2>>(totalAABB, shapes.clone());
		}
		else
		{
			if (currentSorting != bestAxis)
			{
				shapes.sortByCentroid(bestAxis);
			}

			// Split objects in [0 .. bestSplit) and [bestSplit .. shapeCount)
			auto[listA, listB] = shapes.split(bestSplit);
			auto node = std::make_unique<BVHNode<ShapeList, TRayHitInfo, 2>>(totalAABB);
			node->setChild(0, buildNode(*listA, currentSorting));
			node->setChild(1, buildNode(*listB, currentSorting));
			return node;
		}
	}

public:
	static BVH<IShapeList<TRayHitInfo>, TRayHitInfo, 2> buildBVH(IShapeList<TRayHitInfo>& shapes)
	{
		BVHBuilder builder(shapes.count(), 1, 1);//TODO: intersect cost
		shapes.sortByCentroid(Axis::x);
		auto rootNode = builder.buildNode(shapes, Axis::x);
		return BVH<ShapeList, TRayHitInfo, 2>(std::move(rootNode));
	}
};

