#include "../testUtil.h"
#include "IO/InputParser.h"
#include "routing/DijkstraOverlap.h"
#include <gtest/gtest.h>
#include <routing/KShortest.h>

class RoutingTest : public testing::Test
{
protected:
    routing::KShortest kShortestNavigator_;
    routing::DijkstraOverlap dijkstraOverlapNavigator_;
};

TEST_F(RoutingTest, trivial_graph)
{
    const auto graph = io::parseNetworkGraph("../../test/test_data/simple_network.txt").value();

    constexpr auto source = common::NetworkNodeID{0};
    constexpr auto destination = common::NetworkNodeID{3};

    const auto dijkstraOverlapRoutes = dijkstraOverlapNavigator_.findRoutes(source, destination, graph, 2);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 2);

    const auto kShortestRoutes = kShortestNavigator_.findRoutes(source, destination, graph, 2);
    ASSERT_EQ(kShortestRoutes.size(), 2);

    ASSERT_EQ(dijkstraOverlapRoutes, kShortestRoutes);
}

TEST_F(RoutingTest, trivial_graph_ask_for_too_many_routes)
{
    const auto graph = io::parseNetworkGraph("../../test/test_data/simple_network.txt").value();

    constexpr auto source = common::NetworkNodeID{0};
    constexpr auto destination = common::NetworkNodeID{3};

    const auto dijkstraOverlapRoutes = dijkstraOverlapNavigator_.findRoutes(source, destination, graph, 3);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 2);

    const auto kShortestRoutes = kShortestNavigator_.findRoutes(source, destination, graph, 3);
    ASSERT_EQ(kShortestRoutes.size(), 2);

    ASSERT_EQ(dijkstraOverlapRoutes, kShortestRoutes);
}

TEST_F(RoutingTest, check_for_expected_different_results)
{
    auto graph = io::parseNetworkGraph("../../test/test_data/routing_graph_2.txt").value();

    constexpr auto source = common::NetworkNodeID{0};
    constexpr auto destination = common::NetworkNodeID{30};

    auto dijkstraOverlapRoutes = dijkstraOverlapNavigator_.findRoutes(source, destination, graph, 3);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 3);

    ASSERT_EQ(dijkstraOverlapRoutes[0].size(), 7);
    ASSERT_EQ(dijkstraOverlapRoutes[1].size(), 10);
    ASSERT_EQ(dijkstraOverlapRoutes[2].size(), 11);

    fmt::print("DijkstraOverlap routes:\n");
    for(const auto& route : dijkstraOverlapRoutes) {
        print_route(route, graph, source);
    }

    auto kShortestRoutes = kShortestNavigator_.findRoutes(source, destination, graph, 3);
    ASSERT_EQ(kShortestRoutes.size(), 3);
    fmt::print("kShortest Path routes:\n");
    for(const auto& route : kShortestRoutes) {
        ASSERT_EQ(route.size(), 7);
        print_route(route, graph, source);
    }

    ASSERT_NE(dijkstraOverlapRoutes, kShortestRoutes);
}


TEST_F(RoutingTest, ieee14bus)
{
    auto graph = io::parseNetworkGraph("../../test/test_data/IEEE14bus_graph.txt").value();

    auto dijkstraOverlapRoutes = kShortestNavigator_.findRoutes(common::NetworkNodeID{28}, common::NetworkNodeID{32}, graph, 3);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 3);
    ASSERT_EQ(dijkstraOverlapRoutes[0].size(), 4);
    ASSERT_EQ(dijkstraOverlapRoutes[1].size(), 6);
    ASSERT_EQ(dijkstraOverlapRoutes[2].size(), 7);

    dijkstraOverlapRoutes = kShortestNavigator_.findRoutes(common::NetworkNodeID{21}, common::NetworkNodeID{26}, graph, 3);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 3);
    ASSERT_EQ(dijkstraOverlapRoutes[0].size(), 7);
    ASSERT_EQ(dijkstraOverlapRoutes[1].size(), 7);
    ASSERT_EQ(dijkstraOverlapRoutes[2].size(), 8);


    dijkstraOverlapRoutes = kShortestNavigator_.findRoutes(common::NetworkNodeID{29}, common::NetworkNodeID{17}, graph, 3);
    ASSERT_EQ(dijkstraOverlapRoutes.size(), 3);
    ASSERT_EQ(dijkstraOverlapRoutes[0].size(), 6);
    ASSERT_EQ(dijkstraOverlapRoutes[1].size(), 7);
    ASSERT_EQ(dijkstraOverlapRoutes[2].size(), 8);
}
