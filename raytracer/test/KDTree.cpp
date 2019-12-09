#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "photonmapping/KDTree.h"
#include "photonmapping/Photon.h"

using namespace testing;

struct Entry
{
    Point pos;

    Entry(Point p) : pos(p) {}

    const Point& getPos() const
    {
        return pos;
    }
};

TEST(KDTree, NNearestElements1)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(Entry(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(2);
    Point target(-4, 0, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(resultsList[0], nodeB);
    ASSERT_EQ(resultsList[1], nodeA);
}

TEST(KDTree, NNearestElements2)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,5,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(Entry(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(2);
    Point target(5, 4, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(resultsList[0], nodeC);
    ASSERT_EQ(resultsList[1], nodeA);
}

TEST(KDTree, NNearestElements3)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-1,4,0))));
    auto* nodeB = &node->getChild(0).getContent();
    node->setChild(1, std::make_unique<NodeType>(Entry(Point(5,0,0))));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(1);
    Point target(1, 4, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeB->pos).norm());
    ASSERT_EQ(resultsList[0], nodeB);
}

TEST(KDTree, NNearestElements4)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(Entry(Point(2,0,0)));
    treeNodeC->setChild(1, std::make_unique<NodeType>(Entry(Point(2,1,0))));
    treeNodeC->setAxis(Axis::y);
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(1);
    Point target(2, -1, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeC->pos).norm());
    ASSERT_EQ(resultsList[0], nodeC);
}

TEST(KDTree, NNearestElements5)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(Entry(Point(2,0,0)));
    treeNodeC->setAxis(Axis::y);
    treeNodeC->setChild(1, std::make_unique<NodeType>(Entry(Point(2,1,0))));
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(1);
    Point target(-1, 1, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 1);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeA->pos).norm());
    ASSERT_EQ(resultsList[0], nodeA);
}

TEST(KDTree, NNearestElements6)
{
    using NodeType = KDTreeNode<Entry, &Entry::getPos, unique_pointer>;
    auto node = std::make_unique<NodeType>(Entry(Point(0,0,0)));
    auto* nodeA = &node->getContent();
    node->setAxis(Axis::x);
    node->setChild(0, std::make_unique<NodeType>(Entry(Point(-5,0,0))));
    auto* nodeB = &node->getChild(0).getContent();
    auto treeNodeC = std::make_unique<NodeType>(Entry(Point(3,0,0)));
    treeNodeC->setChild(1, std::make_unique<NodeType>(Entry(Point(1,1,0))));
    treeNodeC->setAxis(Axis::y);
    node->setChild(1, std::move(treeNodeC));
    auto* nodeC = &node->getChild(1).getContent();
    auto* nodeD = &node->getChild(1).getChild(1).getContent();

    KDTree<Entry, &Entry::getPos> tree(std::move(node), 3);

    std::vector<const Entry*> resultsList(2);
    Point target(3, 1, 0);
    auto [nbPhotonsFound, maxDist] = tree.getElementsNearestTo(target, resultsList.size(), resultsList);

    ASSERT_EQ(nbPhotonsFound, 2);
    ASSERT_FLOAT_EQ(maxDist, (target - nodeD->pos).norm());
    ASSERT_EQ(resultsList[0], nodeC);
    ASSERT_EQ(resultsList[1], nodeD);
}