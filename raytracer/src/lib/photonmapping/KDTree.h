#pragma once

#include <array>
#include <memory>
#include <optional>
#include <algorithm>
#include <variant>
#include <queue>
#include <limits>
#include "math/Vector3.h"
#include "shape/AABB.h"
#include "photonmapping/Photon.h"
#include "math/Axis.h"
#include "utility/raw_pointer.h"
#include "utility/unique_ptr_template.h"

template<typename TContent, const Point&(TContent::*posAccessor)() const, template<class> class SubNodePointer>
class KDTreeNode
{
public:
    using ChildArray = std::array<SubNodePointer<KDTreeNode>, 2>;

    explicit KDTreeNode(TContent leafData)
            : children(ChildArray()), value(std::move(leafData)), axis(Axis::x)
    {}

    KDTreeNode(TContent leafData, Axis axis)
        : children(ChildArray()), value(std::move(leafData)), axis(axis)
    {}

    KDTreeNode()
        : children(ChildArray()), axis(Axis::x)
    {}

    bool isLeafNode() const
    {
        return !hasChild(0) && !hasChild(1);
    }

    bool hasChild(int idx) const
    {
        return children[idx] != nullptr;
    }

    const KDTreeNode& getChild(int idx) const
    {
        return *(children[idx]);
    }

    void setChild(int idx, SubNodePointer<KDTreeNode> child)
    {
        children[idx] = std::move(child);
    }

    SubNodePointer<KDTreeNode>& getChildPtr(int i)
    {
        return children[i];
    }

    Axis getAxis() { return axis; }

    void setAxis(Axis axis)
    {
        this->axis = axis;
    }

    const TContent& getContent() const
    {
        return value;
    }

    const Point& getPosition() const
    {
        return (value.*posAccessor)();
    }

    template<bool targetIsInsideRange, typename Filter>
    float getElementsNearestTo(const Point& target, unsigned int count, Filter filter, std::vector<const TContent*>& resultsList, float searchRadius, const AABB& curRange) const
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

        const auto& position = getPosition();

        if(isLeafNode())
        {
            if((position - target).norm() < searchRadius && filter(this->value))
            {
                searchRadius = insert(&value, resultsList, count, target);
            }

            return searchRadius;
        }

        auto axisIdx = static_cast<int>(axis);
        auto targetContainingChildIdx = position[axisIdx] < target[axisIdx] ? 1 : 0;

        // Calculate the AABB of each child node
        std::array<AABB, 2> subRanges = curRange.split(axis, position[axisIdx]);

        if(hasChild(targetContainingChildIdx))
        {
            searchRadius = getChild(targetContainingChildIdx).template getElementsNearestTo<targetIsInsideRange, Filter>(target, count, filter, resultsList, searchRadius, subRanges[targetContainingChildIdx]);
        }

        if((position - target).norm() < searchRadius && filter(this->value))
        {
            searchRadius = insert(&value, resultsList, count, target);
        }

        auto otherChildIdx = (targetContainingChildIdx + 1) % 2;
        if(hasChild(otherChildIdx))
        {
            float distanceToSplitPlane;
            if(targetIsInsideRange)
            {
                distanceToSplitPlane = abs(position[axisIdx] - target[axisIdx]);
            }
            else
            {
                Point projection = curRange.projectPointOntoAABB(target);
                distanceToSplitPlane = (projection - target).norm();
            }

            if(distanceToSplitPlane <= searchRadius)
            {
                searchRadius = getChild(otherChildIdx).template getElementsNearestTo<false, Filter>(target, count, filter, resultsList, searchRadius, subRanges[otherChildIdx]);
            }
        }

        return searchRadius;
    }

    void getElementsInRadiusFrom(const Point& target, float radius, std::vector<const TContent*>& resultsList, const AABB& curRange) const
    {
        /*
         NEIGHBOURS IN FIXED RADIUS:
         Recursively step down tree:
         At each step, check if dist(current node, input pos) < radius (append to result list if true)
         Recurse into child that contains input pos.
         if radius > min(dist(inputPos, proj(inputPos, split plane start))), dist(inputPos, proj(inputPos, split plane end))))
            Recurse into other child
         */

        // Check if this node has a matching value
        const auto& position = getPosition();
        if((position - target).norm() < radius)
        {
            resultsList.push_back(&value);
        }

        if(isLeafNode())
        {
            return;
        }

        // Calculate the AABB of each child node
        auto axisIdx = static_cast<int>(axis);
        std::array<AABB, 2> subRanges = curRange.split(axis, position[axisIdx]);

        // Check if the first child node has a matching value
        auto targetContainingChildIdx = position[axisIdx] < target[axisIdx] ? 1 : 0;
        if(hasChild(targetContainingChildIdx))
        {
            getChild(targetContainingChildIdx).getElementsInRadiusFrom(target, radius, resultsList, subRanges[targetContainingChildIdx]);
        }

        auto otherChildIdx = (targetContainingChildIdx + 1) % 2;
        if(hasChild(otherChildIdx))
        {
            // Check if the search radius intersects the second child
            //'projection' is the projection of the target point onto the splitting plane,.
            //The splitting "plane" is really a rectangle along 'axis' through 'position', bounded by 'curRange'.
            Point projection = curRange.projectPointOntoAABB(target);
            auto projectionDist = (position - projection).norm();

            if(radius >= projectionDist)
            {
                // Check if the second child node has a matching value
                getChild(otherChildIdx).getElementsInRadiusFrom(target, radius, resultsList, subRanges[otherChildIdx]);
            }
        }
    }

private:
    ChildArray children;
    TContent value;
    Axis axis;

    // inserts item into list sorted by distance to target and removes last element to maintain list length
    static float insert(const TContent* elem, std::vector<const TContent*>& resultsList, unsigned int count, const Point& target)
    {
        auto begin = resultsList.begin() + (resultsList.size() - count);
        auto it = std::lower_bound(begin, resultsList.end(), elem, [&target](const TContent* c1, const TContent* c2){
            auto dist1 = c1 == nullptr ? 1E99 : ((c1->*posAccessor)() - target).squaredNorm();
            auto dist2 = c2 == nullptr ? 1E99 : ((c2->*posAccessor)() - target).squaredNorm();
            return dist1 < dist2;
        });
        resultsList.insert(it, elem);
        resultsList.pop_back();
        auto lastElemPtr = resultsList.back();
        if(lastElemPtr == nullptr)
        {
            return std::numeric_limits<float>::max();
        }
        const auto& lastElemPos = (lastElemPtr->*posAccessor)();
        return (lastElemPos - target).norm();
    }
};

template<typename TContent, const Point&(TContent::*posAccessor)() const>
class KDTree
{
public:
    using LinkedNode = KDTreeNode<TContent, posAccessor, unique_pointer>;
    using PackedNode = KDTreeNode<TContent, posAccessor, raw_pointer>;

    KDTree(std::unique_ptr<LinkedNode> rootNode, size_t treeSize)
        : nodes(std::move(rootNode)), treeSize(treeSize)
    { }

    size_t getSize() const
    {
        return treeSize;
    }

    void pack()
    {
        if(getSize() == 0)
        {
            return;
        }

        if(!std::holds_alternative<std::unique_ptr<LinkedNode>>(nodes))
        {
            throw std::runtime_error("KDTree is already packed or has no nodes");
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

            packedNodes.emplace_back(node->getContent(), node->getAxis());
            if(!node->isLeafNode())
            {
                for(int i = 0; i < 2; i++)
                {
                    if(node->hasChild(i))
                    {
                        remainingNodes.push(std::make_tuple(std::move(node->getChildPtr(i)), &packedNodes.back(), i));
                    }
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

    template<typename Filter>
    float getElementsNearestTo(const Point& target, unsigned int count, Filter filter, std::vector<const TContent*>& resultsList) const
    {
        if(std::holds_alternative<std::unique_ptr<LinkedNode>>(nodes))
        {
            return std::get<std::unique_ptr<LinkedNode>>(nodes)->template getElementsNearestTo<true, Filter>(target, count, filter, resultsList, std::numeric_limits<float>::max(), AABB::MAX_RANGE);
        }
        else
        {
            return std::get<std::vector<PackedNode>>(nodes)[0].template getElementsNearestTo<true, Filter>(target, count, filter, resultsList, std::numeric_limits<float>::max(), AABB::MAX_RANGE);
        }
    }

    float getElementsNearestTo(const Point& target, unsigned int count, std::vector<const TContent*>& resultsList) const
    {
        auto filter = [](const TContent& elem){return true;};
        return getElementsNearestTo(target, count, filter, resultsList);
    }

    void getElementsInRadiusFrom(const Point& target, float radius, std::vector<const TContent*>& resultsList) const
    {
        if(std::holds_alternative<std::unique_ptr<LinkedNode>>(nodes))
        {
            return std::get<std::unique_ptr<LinkedNode>>(nodes)->getElementsInRadiusFrom(target, radius, resultsList, AABB::MAX_RANGE);
        }
        else
        {
            return std::get<std::vector<PackedNode>>(nodes)[0].getElementsInRadiusFrom(target, radius, resultsList, AABB::MAX_RANGE);
        }
    }

private:
    std::variant<
            std::unique_ptr<LinkedNode>,
            std::vector<PackedNode>
    > nodes;
    size_t treeSize;
};
