// all the includes you want to use before the gtest include
#include <IO/InputParser.h>
#include <graph/GraphStructOperations.h>
#include <gtest/gtest.h>
#include <routing/DijkstraOverlap.h>


TEST(GraphCreationTest, LoadNetworkGraphSuccessful)
{
    const auto* const network_graph_path = "../../test/test_data/simple_network.txt";
    const auto graph = io::parseNetworkGraph(network_graph_path).value();

    ASSERT_EQ(graph.getNumberOfNetworkNodes(), 5);
    ASSERT_EQ(graph.getNumberOfEgressQueues(), 10);

    std::unordered_map<common::NetworkNodeID, std::vector<common::NetworkNodeID>> neighbors;
    neighbors[common::NetworkNodeID{0}] = {common::NetworkNodeID{1}};
    neighbors[common::NetworkNodeID{1}] = {common::NetworkNodeID{0}, common::NetworkNodeID{2}, common::NetworkNodeID{4}};
    neighbors[common::NetworkNodeID{2}] = {common::NetworkNodeID{1}, common::NetworkNodeID{3}, common::NetworkNodeID{4}};
    neighbors[common::NetworkNodeID{3}] = {common::NetworkNodeID{2}};
    neighbors[common::NetworkNodeID{4}] = {common::NetworkNodeID{1}, common::NetworkNodeID{2}};

    for(const auto& [node_id, neighbor_list] : neighbors) {

        // check if the degree of the network nodes is correct
        ASSERT_EQ(graph.getEgressQueuesOf(node_id).size(), neighbor_list.size());

        for(const auto& egress_queue : graph.getEgressQueuesOf(node_id)) {
            // search for the neighbor id
            auto iterator = std::ranges::find(neighbor_list, egress_queue.destination);
            ASSERT_TRUE(iterator != neighbor_list.end());
        }
    }
}

TEST(GraphCreationTest, LoadGraphError)
{
    const auto* const network_graph_path = "../../test/test_data/notExisting.txt";
    const auto graph_optional = io::parseNetworkGraph(network_graph_path);
    ASSERT_FALSE(graph_optional.has_value());
}

TEST(GraphCreationTest, LoadFlows)
{
    const auto* const network_graph_path = "../../test/test_data/simple_network.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    const auto* const scenario_path = "../../test/test_data/simple_scenario.json";
    auto time_steps = io::parseScenario(scenario_path);

    for(auto& flow : time_steps[0].add_flows) {
        graph.addFlow(std::move(flow));
    }

    ASSERT_EQ(graph.getNumberOfFlows(), 5);

    // create configs
    routing::DijkstraOverlap navigator;
    for(const auto& [key, flow] : graph.getFlows()) {
        constexpr auto number_of_configs_per_flow = 3;
        auto routes = navigator.findRoutes(flow.source, flow.destination, graph, number_of_configs_per_flow);

        for(const auto& path : routes) {
            graph_struct_operations::insertConfiguration(graph, flow.id, path);
        }
    }
    // only 2 configs are created per Flow, since there are only two distinct paths
    ASSERT_EQ(graph.getNumberOfConfigs(), 10);
}

TEST(GraphCreationTest, RemoveDuplicatesTest)
{
    const auto* const network_graph_path = "../../test/test_data/directed_graph.txt";
    const auto graph = io::parseNetworkGraph(network_graph_path).value();

    ASSERT_EQ(graph.getNumberOfNetworkNodes(), 7);
    ASSERT_EQ(graph.getNumberOfEgressQueues(), 12);
}

TEST(GraphCreationTest, LoadIEEE14busSystem)
{
    const auto* const network_graph_path = "../../test/test_data/IEEE14bus_graph.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    ASSERT_EQ(graph.getNumberOfNetworkNodes(), 36);
    ASSERT_EQ(graph.getNumberOfEgressQueues(), 84);

    for(std::size_t i = 0; i < 36; ++i) {
        auto x = graph.getEgressQueuesOf(common::NetworkNodeID{i});
        ASSERT_TRUE(x.size() > 0);
        ASSERT_TRUE(x.size() <= 6);
    }

    // check some out neighbors
    auto neighbors_0 = graph.getEgressQueuesOf(common::NetworkNodeID{0});
    ASSERT_EQ(neighbors_0.size(), 3);
    ASSERT_EQ(neighbors_0[0].destination.get(), 1);
    ASSERT_EQ(neighbors_0[1].destination.get(), 4);
    ASSERT_EQ(neighbors_0[2].destination.get(), 17);

    auto neighbors_2 = graph.getEgressQueuesOf(common::NetworkNodeID{2});
    ASSERT_EQ(neighbors_2.size(), 4);
    ASSERT_EQ(neighbors_2[0].destination.get(), 1);
    ASSERT_EQ(neighbors_2[1].destination.get(), 3);
    ASSERT_EQ(neighbors_2[2].destination.get(), 19);
    ASSERT_EQ(neighbors_2[3].destination.get(), 23);

    auto neighbors_35 = graph.getEgressQueuesOf(common::NetworkNodeID{35});
    ASSERT_EQ(neighbors_35.size(), 1);
    ASSERT_EQ(neighbors_35[0].destination.get(), 16);

    auto neighbors_11 = graph.getEgressQueuesOf(common::NetworkNodeID{11});
    ASSERT_EQ(neighbors_11.size(), 3);
    ASSERT_EQ(neighbors_11[0].destination.get(), 5);
    ASSERT_EQ(neighbors_11[1].destination.get(), 12);
    ASSERT_EQ(neighbors_11[2].destination.get(), 30);
}
