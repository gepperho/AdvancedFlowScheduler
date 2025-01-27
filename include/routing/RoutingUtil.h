#pragma once

#include <optional>
#include <queue>
#include <routing/AbstractNavigation.h>
#include <unordered_set>


namespace routing {

using WeightMap = std::map<common::NetworkQueueID, int64_t>;

inline auto getDijkstraShortestPath(const common::NetworkNodeID source, const common::NetworkNodeID destination,
                                    const MultiLayeredGraph &network,
                                    const WeightMap &increased_edge_weights)
    -> std::vector<common::NetworkQueueID>
{
    using NodeSet = std::unordered_set<common::NetworkNodeID>;
    using NodeDistancePair = std::pair<common::NetworkNodeID, std::int64_t>;

    // setup
    auto link_from_predecessor = std::vector<std::optional<common::NetworkQueueID>>(network.getNumberOfNetworkNodes());
    auto predecessor_id = std::vector<common::NetworkNodeID>(network.getNumberOfNetworkNodes());

    auto comp_less_dist = [](const auto &lhs, const auto &rhs) {
        return lhs.second > rhs.second;
    };
    std::priority_queue<NodeDistancePair, std::vector<NodeDistancePair>, decltype(comp_less_dist)> frontier_queue(comp_less_dist);

    auto frontier_distances = std::vector<int64_t>(network.getNumberOfNetworkNodes(), INT64_MAX - 1000000);
    frontier_distances[source.get()] = std::int64_t{0};
    frontier_queue.emplace(source.get(), std::int64_t{0});

    NodeSet checked_nodes;
    while(!frontier_queue.empty()) {

        const auto [current_node, current_distance] = frontier_queue.top();
        frontier_queue.pop();

        if(frontier_distances[destination.get()] < current_distance) {
            // useless path
            continue;
        }

        if(checked_nodes.contains(current_node)) {
            /* this filters already expanded nodes, since we add duplicates in emplace */
            continue;
        }

        // expand node
        for(const auto &out_link : network.getEgressQueuesOf(current_node)) {
            // get edge weight
            int64_t edge_weight{1};
            if(increased_edge_weights.contains(out_link.id)) {
                edge_weight = increased_edge_weights.at(out_link.id);
            }

            if(auto distance_to_next_hop = frontier_distances[current_node.get()] + edge_weight;
               distance_to_next_hop < frontier_distances[out_link.destination.get()]) {
                frontier_distances[out_link.destination.get()] = distance_to_next_hop;

                link_from_predecessor[out_link.destination.get()] = out_link.id;
                predecessor_id[out_link.destination.get()] = current_node;
                /* update frontier_queue,
                 * note that this can add  duplicates with lower frontier_distances
                 */
                frontier_queue.emplace(out_link.destination, distance_to_next_hop);
            }
        }
        checked_nodes.insert(current_node);
    }

    /* extract path */
    std::vector<common::NetworkQueueID> path;
    auto current_node = destination;

    while(current_node.operator!=(source)) {
        auto current_link = link_from_predecessor[current_node.get()].value();
        path.emplace_back(current_link);
        const auto predecessor = predecessor_id[current_node.get()];

        current_node = predecessor;
    }

    std::ranges::reverse(path);

    return path;
}
} // namespace routing
