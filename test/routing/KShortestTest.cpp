#include "IO/InputParser.h"
#include <gtest/gtest.h>
#include <routing/KShortest.h>

class KShortestTest : public testing::Test
{
protected:
    void SetUp() override
    {
        const auto* const network_graph_path = "../../test/test_data/routing_graph.txt";
        graph = io::parseNetworkGraph(network_graph_path).value();

        ASSERT_EQ(graph.getNumberOfNetworkNodes(), 11);
        ASSERT_EQ(graph.getNumberOfEgressQueues(), 12 * 2);
    }

    MultiLayeredGraph graph;
    routing::KShortest navigator;
};

TEST_F(KShortestTest, name)
{
    ASSERT_EQ(navigator.name(), "kShortest");
}

TEST_F(KShortestTest, calculateRoutes)
{
    auto routes_0_to_7 = navigator.findRoutes(common::NetworkNodeID{0}, common::NetworkNodeID{7}, graph, 3);
    ASSERT_EQ(routes_0_to_7[0].size(), 4);
    ASSERT_EQ(routes_0_to_7[1].size(), 4);
    ASSERT_EQ(routes_0_to_7[2].size(), 5);

    // make sure the two shorter paths are not identical
    ASSERT_NE(routes_0_to_7[0][2].get(), routes_0_to_7[1][2].get());

    // make sure only three paths are calculated since there are only three distinct paths
    ASSERT_EQ(routes_0_to_7.size(), 3);
}


TEST_F(KShortestTest, calculateOneHopRoutes)
{
    const auto routes_1_to_3 = navigator.findRoutes(common::NetworkNodeID{1}, common::NetworkNodeID{3}, graph, 3);
    ASSERT_EQ(routes_1_to_3[0].size(), 1);
    ASSERT_EQ(routes_1_to_3[1].size(), 3);
    ASSERT_EQ(routes_1_to_3[2].size(), 4);
}

TEST_F(KShortestTest, calculateNoHopRoutes)
{
    const auto routes_1_to_1 = navigator.findRoutes(common::NetworkNodeID{1}, common::NetworkNodeID{1}, graph, 3);
    ASSERT_EQ(routes_1_to_1[0].size(), 0);
    ASSERT_EQ(routes_1_to_1.size(), 1);
}

TEST_F(KShortestTest, calculateJustTwoRoutes)
{
    const auto routes_0_to_7 = navigator.findRoutes(common::NetworkNodeID{0}, common::NetworkNodeID{7}, graph, 2);
    ASSERT_EQ(routes_0_to_7[0].size(), 4);
    ASSERT_EQ(routes_0_to_7[1].size(), 4);

    // make sure the two paths are not identical
    ASSERT_NE(routes_0_to_7[0][2].get(), routes_0_to_7[1][2].get());

    // make sure only three paths are calculated since there are only three distinct paths
    ASSERT_EQ(routes_0_to_7.size(), 2);
}