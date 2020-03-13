#pragma once

#include <iostream>
#include <future>
#include <mutex>
#include "KDTree.h"
#include "Photon.h"
#include "utility/unique_ptr_template.h"

#ifndef NO_TBB
#include <tbb/tbb.h>
class KDTreeNodeBuildingTask;
#endif

struct PhotonListSlice
{
    using iterator = PhotonList::iterator;
    iterator begin;
    iterator end;

    PhotonListSlice(iterator begin, iterator end)
        : begin(begin), end(end)
    {}

    void sortByAxis(Axis axis, bool allowParallel)
    {
        auto axisIdx = static_cast<int>(axis);
        auto comparator = [axisIdx](const Photon& p1, const Photon& p2){
            return p1.getPosition()[axisIdx] < p2.getPosition()[axisIdx];
        };

#ifndef NO_TBB
        if(allowParallel)
        {
            tbb::parallel_sort(begin, end, comparator);
        }
        else
#endif
        {
            std::sort(begin, end, comparator);
        }
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

    std::variant<NodePtr, IncompleteNode> buildNode(PhotonListSlice& photons, Axis presortedAxis, bool allowParallel);

    NodePtr buildTree(PhotonListSlice& photons, Axis presortedAxis);

#ifndef NO_TBB
    NodePtr buildTreeThreaded(PhotonListSlice& photons, Axis presortedAxis);
#endif

    inline static std::mutex exec_mutex;
};

#ifndef NO_TBB
class KDTreeNodeBuildingTask : public tbb::task
{
public:
    using Node = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    using NodePtr = std::unique_ptr<Node>;

    std::reference_wrapper<KDTreeBuilder> builder;
    std::reference_wrapper<PhotonListSlice> photons;
    unsigned int curTreeDepth;
    Axis presortedAxis;
    NodePtr* outputTreePtr;

    KDTreeNodeBuildingTask(KDTreeBuilder& builder, PhotonListSlice& photons, Axis presortedAxis, unsigned int curTreeDepth, /* OUTPUT */ NodePtr* outputTree)
            : builder(std::ref(builder)), photons(std::ref(photons)), presortedAxis(presortedAxis), curTreeDepth(curTreeDepth), outputTreePtr(outputTree)
    { }

    task* execute() override
    {
        auto& tree = *outputTreePtr;

        if(photons.get().count() == 0)
        {
            tree = nullptr;
        }
        else
        {
            auto result = builder.get().buildNode(photons.get(), presortedAxis, curTreeDepth < 2);
            if (std::holds_alternative<NodePtr>(result))
            {
                tree = std::move(std::get<NodePtr>(result));
            }
            else
            {
                auto& incompleteNode = std::get<typename KDTreeBuilder::IncompleteNode>(result);

                NodePtr subNodeA;
                NodePtr subNodeB;

                bool useSubTasksForSubLists = incompleteNode.leftSubList.count() > 1000 || incompleteNode.rightSubList.count() > 1000;
                if(useSubTasksForSubLists)
                {
                    KDTreeNodeBuildingTask& a = *new(allocate_child()) KDTreeNodeBuildingTask(builder, incompleteNode.leftSubList, incompleteNode.presortedAxis, curTreeDepth + 1, &subNodeA);
                    KDTreeNodeBuildingTask& b = *new(allocate_child()) KDTreeNodeBuildingTask(builder, incompleteNode.rightSubList, incompleteNode.presortedAxis, curTreeDepth + 1, &subNodeB);

                    set_ref_count(3);

                    spawn(b);
                    spawn_and_wait_for_all(a);
                }
                else
                {
                    subNodeA = builder.get().buildTree(incompleteNode.leftSubList, incompleteNode.presortedAxis);
                    subNodeB = builder.get().buildTree(incompleteNode.rightSubList, incompleteNode.presortedAxis);
                }

                tree = std::move(incompleteNode.node);
                tree->setChild(0, std::move(subNodeA));
                tree->setChild(1, std::move(subNodeB));
            }
        }

        return nullptr;
    }
};
#endif

/**************************************/
/**** KDTreeBuilder implementation ****/
/**************************************/

KDTreeBuilder::KDTreeBuilder() = default;

std::variant<typename KDTreeBuilder::NodePtr, typename KDTreeBuilder::IncompleteNode>
        KDTreeBuilder::buildNode(PhotonListSlice& photons, Axis presortedAxis, bool allowParallel)
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
	photons.sortByAxis(currentSorting, allowParallel);
	auto curElemIdx = photonCount / 2;

    // Split objects in [0 .. bestSplit) and [bestSplit .. shapeCount)
    auto [listA, listB] = photons.split(curElemIdx);
    return IncompleteNode{ std::make_unique<Node>(*(photons.begin + curElemIdx), currentSorting), listA, listB, currentSorting };
}

std::unique_ptr<typename KDTreeBuilder::Node> KDTreeBuilder::buildTree(PhotonListSlice& photons, Axis presortedAxis)
{
    struct Entry { PhotonListSlice photons; std::unique_ptr<Node>& targetStorage; Axis presortedAxis; };

    std::queue<Entry> todo{};
    std::unique_ptr<Node> root = nullptr;
    todo.push(Entry{photons, root, presortedAxis});

    while(!todo.empty())
    {
        auto& cur = todo.front();

        auto result = buildNode(cur.photons, cur.presortedAxis, false);
        if (std::holds_alternative<NodePtr>(result))
        {
            cur.targetStorage = NodePtr(std::move(std::get<NodePtr>(result)));
        }
        else
        {
            auto& incompleteNode = std::get<IncompleteNode>(result);
            if(incompleteNode.leftSubList.count() > 0)
            {
                todo.push(Entry{incompleteNode.leftSubList, incompleteNode.node->getChildPtr(0), incompleteNode.presortedAxis});
            }
            if(incompleteNode.rightSubList.count() > 0)
            {
                todo.push(Entry{incompleteNode.rightSubList, incompleteNode.node->getChildPtr(1), incompleteNode.presortedAxis});
            }
            cur.targetStorage = std::move(incompleteNode.node);
        }

        todo.pop();
    }

    return std::move(root);
}

/*std::unique_ptr<typename KDTreeBuilder::Node> KDTreeBuilder::buildTree(PhotonListSlice& photons, Axis presortedAxis)
{
    if(photons.count() == 0)
    {
        return nullptr;
    }

    auto result = buildNode(photons, presortedAxis);
    if (std::holds_alternative<NodePtr>(result))
    {
        return NodePtr(std::move(std::get<NodePtr>(result)));
    }
    else
    {
        auto& incompleteNode = std::get<IncompleteNode>(result);
        NodePtr node(std::move(incompleteNode.node));
        node->setChild(0, this->buildTree(incompleteNode.leftSubList, incompleteNode.presortedAxis));
        node->setChild(1, this->buildTree(incompleteNode.rightSubList, incompleteNode.presortedAxis));
        return node;
    }
}*/

#ifndef NO_TBB
std::unique_ptr<typename KDTreeBuilder::Node> KDTreeBuilder::buildTreeThreaded(PhotonListSlice& photons, Axis presortedAxis)
{
    NodePtr node;

    auto& task = *new(tbb::task::allocate_root()) KDTreeNodeBuildingTask(*this, photons, presortedAxis, 0, &node/*, &size*/);
    tbb::task::spawn_root_and_wait(task);
    return std::move(node);
}
#endif

KDTree<Photon, &Photon::getPosition> KDTreeBuilder::build(PhotonList& photons)
{
    auto size = photons.size();
    PhotonListSlice list(photons.begin(), photons.end());
    list.sortByAxis(Axis::x, true);

    std::lock_guard lock(exec_mutex);

    KDTreeBuilder builder{};

#ifdef NO_TBB
    auto rootNode = builder.buildTree(list, Axis::x);
#else
    auto rootNode = builder.buildTreeThreaded(list, Axis::x);
#endif

    return KDTree<Photon, &Photon::getPosition>(std::move(rootNode), size);
}