#pragma once

#include <algorithm>
#include <graph/GraphStructs.h>
#include <graph/MultiLayeredGraph.h>
#include <unordered_set>

namespace graph_struct_operations {

inline auto use(graph_structs::EgressQueue& queue, const common::ConfigurationNodeID user) -> void
{
    queue.used_by.emplace_back(user);
}

inline auto unuse(graph_structs::EgressQueue& queue, const common::ConfigurationNodeID former_user) -> void
{
    std::erase(queue.used_by, former_user);
}

/**
 * creates a new config holding the given path and adds it to the config list and creates all required links.
 * Attention: this method is not thread safe!
 * @param graph
 * @param flow_id
 * @param path
 */
inline auto insertConfiguration(MultiLayeredGraph& graph, const common::FlowNodeID flow_id, const std::vector<common::NetworkQueueID>& path) -> void
{
    /*
     * Note: this method is currently not thread safe, since we call the not thread safe graph.createConfiguration
     */
    auto config_id = graph.insertConfiguration(flow_id, std::move(path));
    auto& current_flow = graph.getFlow(flow_id);

    // link from Flow to config
    current_flow.configurations.emplace_back(config_id);

    for(const auto& path_segment : graph.getConfiguration(config_id).path) {
        // create link from EgressQueue to Configuration
        use(graph.getEgressQueue(path_segment), config_id);
    }
}

inline auto getFlowsOf(MultiLayeredGraph& graph, const graph_structs::EgressQueue& path_segment) -> std::unordered_set<common::FlowNodeID>
{
    std::unordered_set<common::FlowNodeID> set;
    std::ranges::for_each(path_segment.used_by, [&](const auto config_id) {
        set.insert(graph.getConfiguration(config_id).flow);
    });
    return set;
}

} // namespace graph_struct_operations