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

TEST(KDTreeBuilder, )
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
    tree.getElementsNearestTo(Point(-4, 0, 0), resultsList.size(), resultsList);

    ASSERT_EQ(resultsList[0], nodeB);
    ASSERT_EQ(resultsList[1], nodeA);
}