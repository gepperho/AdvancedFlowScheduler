#include "graph/MultiLayeredGraph.h"
#include <fmt/core.h>
#include <graph/GraphStructOperations.h>


MultiLayeredGraph::MultiLayeredGraph()
{
    network_forward_offset_.emplace_back(0);
}

auto MultiLayeredGraph::getNumberOfFlows() const -> std::size_t
{
    return flows_.size();
}

auto MultiLayeredGraph::getFlow(const common::FlowNodeID flow_id) -> graph_structs::Flow&
{
    // are you sure you want to use operator[] here and not .at()
    return flows_.at(flow_id);
}

auto MultiLayeredGraph::getFlow(const common::FlowNodeID flow_id) const -> const graph_structs::Flow&
{
    return flows_.at(flow_id);
}

auto MultiLayeredGraph::getFlows() const -> const robin_hood::unordered_map<common::FlowNodeID, graph_structs::Flow>&
{
    return flows_;
}

auto MultiLayeredGraph::insertNetworkDevice(const std::vector<common::NetworkNodeID>& neighbors) -> void
{
    const auto next_index = network_forward_offset_[network_forward_offset_.size() - 1];
    network_forward_offset_.emplace_back(next_index + neighbors.size());

    const bool end_device = neighbors.size() == 1;

    for(const auto neighbor : neighbors) {
        const auto next_id = network_forward_edges_.size();
        network_forward_edges_.emplace_back(graph_structs::EgressQueue{.id = common::NetworkQueueID{next_id},
                                                                       .destination = neighbor,
                                                                       .end_device = end_device});
    }
}

auto MultiLayeredGraph::getEgressQueuesOf(const common::NetworkNodeID device) const -> std::span<const graph_structs::EgressQueue>
{
    const auto start_offset = network_forward_offset_[device.get()];
    const auto length = network_forward_offset_[device.get() + 1] - start_offset;

    const std::span complete{network_forward_edges_};
    return complete.subspan(start_offset, length);
}

auto MultiLayeredGraph::getEgressQueue(const common::NetworkQueueID queue_id) const -> const graph_structs::EgressQueue&
{
    return network_forward_edges_.at(queue_id.get());
}

auto MultiLayeredGraph::getEgressQueue(const common::NetworkQueueID queue_id) -> graph_structs::EgressQueue&
{
    return network_forward_edges_.at(queue_id.get());
}

auto MultiLayeredGraph::getEgressQueues() const -> std::span<const graph_structs::EgressQueue>
{
    const std::span complete{network_forward_edges_};
    return complete;
}

auto MultiLayeredGraph::addFlow(graph_structs::Flow f) -> void
{
    flows_[f.id] = std::move(f);
}

auto MultiLayeredGraph::removeFlow(common::FlowNodeID const id) -> void
{
    if(not flows_.contains(id)) {
        return;
    }
    const auto& current_flow = flows_.at(id);
    std::ranges::for_each(current_flow.configurations, [&](auto config_id) {
        for(auto link_id : getConfiguration(config_id).path) {
            graph_struct_operations::unuse(getEgressQueue(link_id), config_id);
        }
        config_nodes_.erase(config_id);
    });
    flows_.erase(id);
}


auto MultiLayeredGraph::removeFlows(std::vector<common::FlowNodeID>& flows) -> void
{
    // remove flows
    std::ranges::for_each(flows, [&](auto flow_id) {
        removeFlow(flow_id);
    });
}

auto MultiLayeredGraph::getNumberOfNetworkNodes() const -> std::size_t
{
    return network_forward_offset_.size() - 1;
}

auto MultiLayeredGraph::getNumberOfEgressQueues() const -> std::size_t
{
    return network_forward_edges_.size();
}

auto MultiLayeredGraph::getNumberOfConfigs() const -> std::size_t
{
    return config_nodes_.size();
}

auto MultiLayeredGraph::insertConfiguration(const common::FlowNodeID flow, const std::vector<common::NetworkQueueID>& path) -> common::ConfigurationNodeID
{
    // to make this thread safe we would have to lock the config_nodes_ vector
    auto id = common::ConfigurationNodeID{config_id_counter_};
    ++config_id_counter_;
    config_nodes_[id] = graph_structs::Configuration{.id = id, .flow = flow, .path = std::move(path)};

    return id;
}

auto MultiLayeredGraph::getConfiguration(const common::ConfigurationNodeID id) -> graph_structs::Configuration&
{
    return config_nodes_.at(id);
}
auto MultiLayeredGraph::getConfigurations() const -> const robin_hood::unordered_map<common::ConfigurationNodeID, graph_structs::Configuration>&
{
    return config_nodes_;
}
