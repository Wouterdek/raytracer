#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "photonmapping/KDTree.h"
#include "photonmapping/Photon.h"
#include "photonmapping/KDTreeSearch.h"

using namespace testing;

Photon makePhoton(Point position)
{
    return Photon(std::move(position), Vector3(), Vector3(), RGB::BLACK, false);
}

TEST(KDTreeSearch, NNearestElements1)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(makePhoton(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);

    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();

    //std::vector<const Entry*> resultsList(2);
    Point target(-4, 0, 0);
    //auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;

    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeB);
    ASSERT_EQ(&tree.getNodes()[results.Contents[1]].getContent(), nodeA);
}

TEST(KDTreeSearch, NNearestElements2)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,5,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(makePhoton(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);

    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();

    Point target(5, 4, 0);

    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;


    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeC);
    ASSERT_EQ(&tree.getNodes()[results.Contents[1]].getContent(), nodeA);
}

TEST(KDTreeSearch, NNearestElements3)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-1,4,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(makePhoton(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);
    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();

    Point target(1, 4, 0);


    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;


    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeB->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeB);
}

TEST(KDTreeSearch, NNearestElements4)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(makePhoton(Point(2,0,0)));
    treeNodeC->setChild(1, std::make_unique<NodeType>(makePhoton(Point(2,1,0))));
    treeNodeC->setAxis(Axis::y);
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);

    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();

    Point target(2, -1, 0);

    tree.pack();
    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;


    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeC->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeC);
}

TEST(KDTreeSearch, NNearestElements5)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(makePhoton(Point(2,0,0)));
    treeNodeC->setAxis(Axis::y);
    treeNodeC->setChild(1, std::make_unique<NodeType>(makePhoton(Point(2,1,0))));
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);

    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();

    Point target(-1, 1, 0);

    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;


    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeA);
}

TEST(KDTreeSearch, NNearestElements6)
{
    using NodeType = KDTreeNode<Photon, &Photon::getPosition, unique_pointer>;
    auto node = std::make_unique<NodeType>(makePhoton(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(makePhoton(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(makePhoton(Point(3,0,0)));
    treeNodeC->setChild(1, std::make_unique<NodeType>(makePhoton(Point(1,1,0))));
    treeNodeC->setAxis(Axis::y);
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();
    auto* nodeD = &node->getChild(1).getChild(1).getContent();

    KDTree<Photon, &Photon::getPosition> tree(std::move(node), 3);
    tree.pack();
    nodeA = &tree.getNodes()[0].getContent();
    nodeB = &tree.getNodes()[0].getChild(0).getContent();
    nodeC = &tree.getNodes()[0].getChild(1).getContent();
    nodeD = &tree.getNodes()[0].getChild(1).getChild(1).getContent();

    Point target(3, 1, 0);

    KDTreeSearch::ResultsList results;
    float searchRadius = 1E9;
    KDTreeSearch::GetNearestNeighbours(&tree.getNodes()[0], target, results, searchRadius);
    float maxDist = 0;
    for(unsigned int i = 0; i < results.Count; ++i)
    {
        auto& curPos = tree.getNodes()[results.Contents[i]].getContent().getPosition();
        auto curDist = (curPos - target).norm();
        maxDist = std::max(maxDist, curDist);
    }
    auto nbPhotonsFound = results.Count;


    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeD->pos).norm());
    ASSERT_EQ(&tree.getNodes()[results.Contents[0]].getContent(), nodeC);
    ASSERT_EQ(&tree.getNodes()[results.Contents[1]].getContent(), nodeD);
}