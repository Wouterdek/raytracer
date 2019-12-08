#pragma once

#include "KDTree.h"
#include "utility/array_offset_pointer.h"

class KDTreeSearch
{
public:
    using Node = KDTreeNode<Photon, &Photon::getPosition, array_offset_pointer>;
    static const unsigned int MAX_SEARCH_DEPTH = 30;

private:
    struct AABB
    {
        float minCoord[3];
        float maxCoord[3];
    };

    static void projectPointOntoAABB(const Point& point, const AABB& aabb, Point& projection)
    {
        for(auto i = 0; i < 3; ++i)
        {
            projection[i] = std::clamp(point[i], aabb.minCoord[i], aabb.maxCoord[i]);
        }
    }

public:
    struct ResultsList
    {
        inline static const int Capacity = 20;
        int Contents[Capacity] = {};
        int Count = 0;
    };
private:
    static float insert(ResultsList& resultsList, int elemIdx, const Point& target, const Node* nodes)
    {
        auto dist = (nodes[elemIdx].getContent().getPosition() - target).norm();

        // Find insertion point it
        unsigned int start = 0;
        unsigned int count = resultsList.Count;
        unsigned int step;
        unsigned int it = 0;
        while (count > 0) {
            step = count / 2;
            it = start + step;
            if ((nodes[resultsList.Contents[it]].getContent().getPosition() - target).norm() < dist) {
                start = ++it;
                count -= step + 1;
            }
            else {
                count = step;
            }
        }

        if(it < ResultsList::Capacity)
        {
            // Move values to make space for new value
            for(signed int i = std::min(resultsList.Count-1, ResultsList::Capacity-2); i >= (int)it; --i)
            {
                resultsList.Contents[i+1] = resultsList.Contents[i];
            }
            resultsList.Count = std::min(resultsList.Count+1, ResultsList::Capacity);

            // Set value
            resultsList.Contents[it] = elemIdx;
        }

        // Return largest distance to target (= searchradius)
        if(resultsList.Count < ResultsList::Capacity)
        {
            return INFINITY;
        }else{
            const auto& lastElem = nodes[resultsList.Contents[ResultsList::Capacity-1]].getContent();
            return (lastElem.getPosition() - target).norm();
        }
    }

    struct StackFrame
    {
        unsigned int curNodeI;
        bool rightChildContainsTarget;
        bool hasBeenProcessed;
        AABB aabb;
    };

    static unsigned int TraverseDown(unsigned int startDepth, const Node* nodes, const Point& target, StackFrame stack[])
    {
        unsigned int curDepth = startDepth;
        while(true) //curDepth < MAX_SEARCH_DEPTH-1
        {
            stack[curDepth].hasBeenProcessed = false;

            const Node* cur = &nodes[stack[curDepth].curNodeI];
            if(cur->isLeafNode()){
                break;
            }

            const auto& position = cur->getPosition();
            auto axisIdx = static_cast<int>(cur->getAxis());
            stack[curDepth].rightChildContainsTarget = position[axisIdx] < target[axisIdx];
            if(!cur->hasChild(stack[curDepth].rightChildContainsTarget))
            {
                break;
            }

            stack[curDepth+1].curNodeI = cur->getChildPtr(stack[curDepth].rightChildContainsTarget).offset();
            stack[curDepth+1].aabb.minCoord[0] = stack[curDepth].aabb.minCoord[0];
            stack[curDepth+1].aabb.minCoord[1] = stack[curDepth].aabb.minCoord[1];
            stack[curDepth+1].aabb.minCoord[2] = stack[curDepth].aabb.minCoord[2];
            stack[curDepth+1].aabb.maxCoord[0] = stack[curDepth].aabb.maxCoord[0];
            stack[curDepth+1].aabb.maxCoord[1] = stack[curDepth].aabb.maxCoord[1];
            stack[curDepth+1].aabb.maxCoord[2] = stack[curDepth].aabb.maxCoord[2];


            if(stack[curDepth].rightChildContainsTarget) {
                stack[curDepth+1].aabb.minCoord[axisIdx] = position[axisIdx];
            } else {
                stack[curDepth+1].aabb.maxCoord[axisIdx] = position[axisIdx];
            }

            curDepth++;
        }
        return curDepth;
    }

public:
    static void GetNearestNeighbours(const Node* nodes, const Point& target/*, unsigned int count, std::function<bool()> filter*/, ResultsList& results, float& searchRadius/*, const AABB& curRange*/)
    {
        /*
         N NEAREST NEIGHBOURS:
         Init resultsList with n nulls. Init search radius to +infinite.
         Recursively step down tree, choosing the child that contains the input pos.
         Once the leaf node is reached, start unwinding the stack.
         At each level:
             Check if search radius > dist(current node, input pos).
                If true, do binary search in resultsList to find index to put the element at.
                Insert node at index and remove last elem.
                Update search radius to dist of new last elem to input position.
             Check if search radius >= dist(inputPos, proj(inputPos, split plane)).
                If true, search the child that was not searched yet: (*)
                    check if search radius > dist(current node, input pos). (insert if true, see above)
                    search child node closest to inputPos (single-axis check) by recursing to *
                    if dist(bestNode, inputPos) > min(dist(inputPos, proj(inputPos, split plane start))), dist(inputPos, proj(inputPos, split plane end))))
                        search other child by recursing to *
         */


        StackFrame stack[MAX_SEARCH_DEPTH];
        stack[0].curNodeI = 0; // Start at root
        stack[0].aabb.minCoord[0] = stack[0].aabb.minCoord[1] = stack[0].aabb.minCoord[2] = -INFINITY;
        stack[0].aabb.maxCoord[0] = stack[0].aabb.maxCoord[1] = stack[0].aabb.maxCoord[2] = INFINITY;

        int TreePathLength = TraverseDown(0, nodes, target, stack);

        for(int32_t i = TreePathLength-1; i >= 0; i--)
        {
            StackFrame& cur = stack[i];
            if(cur.hasBeenProcessed)
            {
                continue;
            }

            const Node& curNode = nodes[cur.curNodeI];
            if((curNode.getPosition() - target).norm() < searchRadius /*&& filter()*/)
            {
                searchRadius = insert(results, cur.curNodeI, target, nodes);
            }

            //Check if other child intersects with searchRadius, if so then traverse down into child
            //Upon traversing down, one child will, by construction, always be in range.
            //The other child may not, and should be checked for range. This range check is as follows:
            //Project the target point onto the range AABB *of the child to be checked*,
            //then compare dist(proj, target) to searchradius
            auto otherChildIdx = cur.rightChildContainsTarget ? 0 : 1;
            if(curNode.hasChild(otherChildIdx))
            {
                // Calculate AABB of other child
                auto curAxis = (int)curNode.getAxis();
                if(cur.rightChildContainsTarget)
                {
                    stack[i+1].aabb.minCoord[curAxis] = stack[i].aabb.minCoord[curAxis];
                    stack[i+1].aabb.maxCoord[curAxis] = curNode.getPosition()[curAxis];
                }
                else
                {
                    stack[i+1].aabb.minCoord[curAxis] = curNode.getPosition()[curAxis];
                    stack[i+1].aabb.maxCoord[curAxis] = stack[i].aabb.maxCoord[curAxis];
                }

                Point projection;
                projectPointOntoAABB(target, stack[i+1].aabb, projection);
                float distanceToSplitPlane = (projection - target).norm();

                if(distanceToSplitPlane <= searchRadius)
                {
                    stack[i+1].curNodeI = curNode.getChildPtr(otherChildIdx).offset();
                    TreePathLength = TraverseDown(i+1, nodes, target, stack);
                    i = TreePathLength-1;
                }
            }

            cur.hasBeenProcessed = true;
        }
    }
};

