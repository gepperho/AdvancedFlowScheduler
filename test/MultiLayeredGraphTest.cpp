#include "routing/DijkstraOverlap.h"
#include <IO/InputParser.h>
#include <graph/GraphStructOperations.h>
#include <gtest/gtest.h>


TEST(MultiLayeredGraphTest, expandAndShrinkGraph)
{
    const auto* const network_graph_path = "../../test/test_data/simple_network.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    std::vector path1 = {common::NetworkQueueID{0}, common::NetworkQueueID{1}, common::NetworkQueueID{2}, common::NetworkQueueID{3}};
    std::vector path2 = {common::NetworkQueueID{0}, common::NetworkQueueID{1}, common::NetworkQueueID{4}, common::NetworkQueueID{2}, common::NetworkQueueID{3}};
    std::vector<std::size_t> possible_queue_sizes;

    ASSERT_EQ(graph.getNumberOfFlows(), 0);
    ASSERT_EQ(graph.getNumberOfConfigs(), 0);
    ASSERT_EQ(graph.getNumberOfNetworkNodes(), 5);
    ASSERT_EQ(graph.getNumberOfEgressQueues(), 10);

    // expand graph
    for(std::size_t i = 0; i < 10; ++i) {
        graph_structs::Flow f{.id = common::FlowNodeID{i}, .frame_size = 250, .period = 20};
        graph.addFlow(f);

        graph_struct_operations::insertConfiguration(graph, f.id, path1);
        graph_struct_operations::insertConfiguration(graph, f.id, path2);

        ASSERT_EQ(graph.getNumberOfFlows(), i + 1);
        ASSERT_EQ(graph.getNumberOfConfigs(), (i + 1) * 2);
        ASSERT_EQ(graph.getNumberOfNetworkNodes(), 5);
        ASSERT_EQ(graph.getNumberOfEgressQueues(), 10);
        possible_queue_sizes = {0, graph.getNumberOfFlows(), graph.getNumberOfConfigs()};
        for(auto& queue : graph.getEgressQueues()) {
            ASSERT_TRUE(util::vector_contains(possible_queue_sizes, queue.used_by.size()));
        }
    }

    // shrink graph
    for(std::size_t i = 0; i < 5; ++i) {
        graph.removeFlow(common::FlowNodeID{i + 5});

        ASSERT_EQ(graph.getNumberOfFlows(), 10 - (i + 1));
        ASSERT_EQ(graph.getNumberOfConfigs(), (10 - (i + 1)) * 2);
        ASSERT_EQ(graph.getNumberOfNetworkNodes(), 5);
        ASSERT_EQ(graph.getNumberOfEgressQueues(), 10);

        possible_queue_sizes = {0, graph.getNumberOfFlows(), graph.getNumberOfConfigs()};
        for(auto& queue : graph.getEgressQueues()) {
            ASSERT_TRUE(util::vector_contains(possible_queue_sizes, queue.used_by.size()));
        }
    }
}