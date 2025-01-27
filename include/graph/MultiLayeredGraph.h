#pragma once

#include <Typedefs.h>
#include <graph/GraphStructs.h>
#include <memory>
#include <span>
#include <string>
#include <util/robin_hood.h>
#include <vector>

class MultiLayeredGraph
{
public:
    MultiLayeredGraph();

    [[nodiscard]] auto getNumberOfFlows() const -> std::size_t;

    [[nodiscard]] auto getFlow(common::FlowNodeID flow_id) -> graph_structs::Flow&;

    [[nodiscard]] auto getFlow(common::FlowNodeID flow_id) const -> const graph_structs::Flow&;

    [[nodiscard]] auto getFlows() const -> const robin_hood::unordered_map<common::FlowNodeID, graph_structs::Flow>&;

    [[nodiscard]] auto getNumberOfConfigs() const -> std::size_t;

    [[nodiscard]] auto getNumberOfNetworkNodes() const -> std::size_t;

    [[nodiscard]] auto getNumberOfEgressQueues() const -> std::size_t;

    [[nodiscard]] auto getEgressQueuesOf(common::NetworkNodeID device) const -> std::span<const graph_structs::EgressQueue>;

    /**
     * const version of getEgressQueue
     * @param queue_id
     * @return const reference
     */
    [[nodiscard]] auto getEgressQueue(common::NetworkQueueID queue_id) const -> const graph_structs::EgressQueue&;

    /**
     * non-const version of getEgressQueue
     * @param queue_id
     * @return non-const reference
     */
    [[nodiscard]] auto getEgressQueue(common::NetworkQueueID queue_id) -> graph_structs::EgressQueue&;

    [[nodiscard]] auto getEgressQueues() const -> std::span<const graph_structs::EgressQueue>;

    /**
     * Adds a network device node in the network layer csr with the according edges.
     *
     * Attention! this insert assumes correct formatting and gap less node enumeration.
     * Don't use this function if you are not aware of how it works
     * Attention! this method is not thread safe
     * @param neighbors
     */
    auto insertNetworkDevice(const std::vector<common::NetworkNodeID>& neighbors) -> void;

    /**
     * Adds a Flow struct to the graph.
     * This does not create any configurations, edges, etc.
     * @param f
     */
    auto addFlow(graph_structs::Flow f) -> void;

    auto removeFlow(common::FlowNodeID id) -> void;

    auto removeFlows(std::vector<common::FlowNodeID>& flows) -> void;

    /**
     * creates a Configuration struct and adds it to the graph.
     * Attention: this method is not thread safe!
     * @param flow
     * @param path
     * @return the newly assigned ConfigurationNodeID of the inserted config
     */
    auto insertConfiguration(common::FlowNodeID flow, const std::vector<common::NetworkQueueID>& path) -> common::ConfigurationNodeID;

    auto getConfiguration(common::ConfigurationNodeID id) -> graph_structs::Configuration&;

    auto getConfigurations() const -> const robin_hood::unordered_map<common::ConfigurationNodeID, graph_structs::Configuration>&;


private:
    // Flow layer
    robin_hood::unordered_map<common::FlowNodeID, graph_structs::Flow> flows_;

    // Config layer
    robin_hood::unordered_map<common::ConfigurationNodeID, graph_structs::Configuration> config_nodes_;

    // network layer
    std::vector<std::size_t> network_forward_offset_;
    std::vector<graph_structs::EgressQueue> network_forward_edges_;

    std::size_t config_id_counter_ = 0;
};
