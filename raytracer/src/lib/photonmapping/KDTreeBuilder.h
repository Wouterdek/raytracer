#pragma once

#include <iostream>
#include <future>
#include <mutex>
#include "KDTree.h"
#include "Photon.h"
#include "utility/unique_ptr_template.h"
#include <tbb/tbb.h>

class KDTreeNodeBuildingTask;

using PhotonList = tbb::concurrent_vector<Photon>;
struct PhotonListSlice
{
    using iterator = PhotonList::iterator;
    iterator begin;
    iterator end;

    PhotonListSlice(iterator begin, iterator end)
        : begin(begin), end(end)
    {}

    void sortByAxis(Axis axis)
    {
        auto axisIdx = static_cast<int>(axis);
        std::sort(begin, end, [axisIdx](const Photon& p1, const Photon& p2){
            return p1.getPosition()[axisIdx] < p2.getPosition()[axisIdx];
        });
    }

    //returns two sublists, [0;idx[ and ]idx;end[
    std::pair<PhotonListSlice, PhotonListSlice> split(PhotonList::size_type idx)
    {
        return std::make_pair(
            PhotonListSlice(begin, begin + idx),
            PhotonListSlice(begin + idx + 1, end)
        );
    }

    PhotonList::size_type count()
    {
        return std::distance(begin, end);
    }
};

class KDTreeBuilder
{
public:
    friend class KDTreeNodeBuildingTask;

    static KDTree<Photon, &Photon::getPosition> build(PhotonList& photons);

private:
    using Node = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    using NodePtr = std::unique_ptr<Node>;

    struct IncompleteNode
    {
        NodePtr node;
        PhotonListSlice leftSubList;
        PhotonListSlice rightSubList;
        Axis presortedAxis;
    };

    KDTreeBuilder();

    std::variant<NodePtr, IncompleteNode> buildNode(PhotonListSlice& photons, Axis presortedAxis);

    NodePtr buildTree(PhotonListSlice& photons, Axis presortedAxis);
    std::pair<NodePtr, size_t> buildTreeThreaded(PhotonListSlice& photons, Axis presortedAxis);

    inline static std::mutex exec_mutex;
};

class KDTreeNodeBuildingTask : public tbb::task
{
public:
    using Node = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    using NodePtr = std::unique_ptr<Node>;

    std::reference_wrapper<KDTreeBuilder> builder;
    std::reference_wrapper<PhotonListSlice> photons;
    Axis presortedAxis;
    NodePtr* outputTreePtr;
    size_t* treeSizePtr;

    KDTreeNodeBuildingTask(KDTreeBuilder& builder, PhotonListSlice& photons, Axis presortedAxis, /* OUTPUT */ NodePtr* outputTree, /* OUTPUT */ size_t* treeSize)
            : builder(std::ref(builder)), photons(std::ref(photons)), presortedAxis(presortedAxis), outputTreePtr(outputTree), treeSizePtr(treeSize)
    { }

    task* execute() override
    {
        auto& tree = *outputTreePtr;
        auto& treeSize = *treeSizePtr;

        if(photons.get().count() == 0)
        {
            tree = nullptr;
            treeSize = 0;
        }
        else
        {
            auto result = builder.get().buildNode(photons.get(), presortedAxis);
            if (std::holds_alternative<NodePtr>(result))
            {
                tree = std::move(std::get<NodePtr>(result));
                treeSize = 1;
            }
            else
            {
                auto& incompleteNode = std::get<typename KDTreeBuilder::IncompleteNode>(result);

                NodePtr subNodeA;
                size_t subNodeSizeA;
                NodePtr subNodeB;
                size_t subNodeSizeB;
                KDTreeNodeBuildingTask& a = *new(allocate_child()) KDTreeNodeBuildingTask(builder, incompleteNode.leftSubList, incompleteNode.presortedAxis, &subNodeA, &subNodeSizeA);
                KDTreeNodeBuildingTask& b = *new(allocate_child()) KDTreeNodeBuildingTask(builder, incompleteNode.rightSubList, incompleteNode.presortedAxis, &subNodeB, &subNodeSizeB);

                set_ref_count(3);
                spawn(b);
                spawn_and_wait_for_all(a);

                tree = std::move(incompleteNode.node);
                tree->setChild(0, std::move(subNodeA));
                tree->setChild(1, std::move(subNodeB));
                treeSize = subNodeSizeA + subNodeSizeB + 1;
            }
        }

        return nullptr;
    }
};

/**************************************/
/**** KDTreeBuilder implementation ****/
/**************************************/

KDTreeBuilder::KDTreeBuilder() = default;

std::variant<typename KDTreeBuilder::NodePtr, typename KDTreeBuilder::IncompleteNode>
        KDTreeBuilder::buildNode(PhotonListSlice& photons, Axis presortedAxis)
{
    auto photonCount = photons.count();
    assert(photonCount > 0);

    if (photonCount == 1)
    {
        // Create leaf node
        return std::make_unique<Node>(*photons.begin);
    }
    else if(photonCount == 2)
    {
        auto [listA, listB] = photons.split(0);
        return IncompleteNode{ std::make_unique<Node>(*(photons.begin), presortedAxis), listA, listB, presortedAxis };
    }

    Axis currentSorting = static_cast<Axis>( (static_cast<int>(presortedAxis) + 1) % 3 );
	photons.sortByAxis(currentSorting);
	auto curElemIdx = photonCount / 2;

    // Split objects in [0 .. bestSplit) and [bestSplit .. shapeCount)
    auto [listA, listB] = photons.split(curElemIdx);
    return IncompleteNode{ std::make_unique<Node>(*(photons.begin + curElemIdx), currentSorting), listA, listB, currentSorting };
}

std::unique_ptr<typename KDTreeBuilder::Node> KDTreeBuilder::buildTree(PhotonListSlice& photons, Axis presortedAxis)
{
    if(photons.count() == 0)
    {
        return nullptr;
    }

    auto result = buildNode(photons, presortedAxis);
    if (std::holds_alternative<NodePtr>(result))
    {
        NodePtr node(std::move(std::get<NodePtr>(result)));
        return node;
    }
    else
    {
        auto& incompleteNode = std::get<IncompleteNode>(result);
        NodePtr node(std::move(incompleteNode.node));
        node->setChild(0, this->buildTree(incompleteNode.leftSubList, incompleteNode.presortedAxis));
        node->setChild(1, this->buildTree(incompleteNode.rightSubList, incompleteNode.presortedAxis));
        return node;
    }
}

std::pair<std::unique_ptr<typename KDTreeBuilder::Node>, size_t> KDTreeBuilder::buildTreeThreaded(PhotonListSlice& photons, Axis presortedAxis)
{
    NodePtr node;
    size_t size;

    auto& task = *new(tbb::task::allocate_root()) KDTreeNodeBuildingTask(*this, photons, presortedAxis, &node, &size);
    tbb::task::spawn_root_and_wait(task);
    return std::make_pair(std::move(node), size);
}

KDTree<Photon, &Photon::getPosition> KDTreeBuilder::build(PhotonList& photons)
{
    PhotonListSlice list(photons.begin(), photons.end());
    list.sortByAxis(Axis::x);

    std::lock_guard lock(exec_mutex);

    KDTreeBuilder builder{};
    auto [rootNode, size] = builder.buildTreeThreaded(list, Axis::x);
    return KDTree<Photon, &Photon::getPosition>(std::move(rootNode), size);
}