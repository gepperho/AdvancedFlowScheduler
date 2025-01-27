#include <routing/KShortest.h>
#include <routing/RoutingUtil.h>
#include <util/UtilFunctions.h>

std::string routing::KShortest::name()
{
    return "kShortest";
}

auto routing::KShortest::findRoutes(const common::NetworkNodeID source,
                                    const common::NetworkNodeID destination,
                                    const MultiLayeredGraph& network,
                                    const std::size_t number_of_candidates)
    -> std::vector<std::vector<common::NetworkQueueID>>
{
    WeightMap weight_map;
    std::vector<std::vector<common::NetworkQueueID>> A;
    A.emplace_back(getDijkstraShortestPath(source, destination, network, weight_map));

    std::vector<std::vector<common::NetworkQueueID>> B;

    for(std::size_t k = 1; k < number_of_candidates; ++k) {
        common::NetworkNodeID spurNode = source;
        auto& previous_route = A[k - 1];
        int i = 0;
        for(const auto& hop_id : previous_route) {
            const auto& hop = network.getEgressQueue(hop_id);
            // Spur node is retrieved from the previous k-shortest path, k − 1.
            // handled partly outside the loop
            // The sequence of nodes from the source to the spur node of the previous k-shortest path.
            std::vector rootPath(previous_route.begin(), previous_route.begin() + i);

            for(auto& r : A) {
                // Remove the links that are part of the previous shortest paths which share the same root path.
                const auto end_index = std::min(r.size(), rootPath.size());
                for(std::size_t compare_index = 0; compare_index < end_index; ++compare_index) {
                    if(r[compare_index].get() != rootPath[compare_index].get()) {
                        // continue outer loop
                        goto paths_not_equal_skip;
                    }
                }
                // subpart of route in A and rootPath are equal
                weight_map[r.at(end_index)] = LARGE_NUMBER;
            paths_not_equal_skip:;
            }

            // this shift ensures we don't remove the last destination, e.g., the starting node of the spur path
            common::NetworkNodeID previous = source;
            for(const auto& root_path_hop_id : rootPath) {
                effectivelyRemoveNode(previous, network, weight_map);
                previous = network.getEgressQueue(root_path_hop_id).destination;
            }

            // Calculate the spur path from the spur node to the sink.
            // Consider also checking if any spurPath found
            auto spurPath = getDijkstraShortestPath(spurNode, destination, network, weight_map);
            // filter illegal paths (those using deleted edges)
            const bool valid = not std::ranges::any_of(spurPath, [&](auto& current_hop) {
                return weight_map.contains(current_hop);
            });

            // Entire path is made up of the root path and spur path.
            rootPath.insert(rootPath.end(), spurPath.begin(), spurPath.end());
            // Add the potential k-shortest path to the heap.
            if(valid and not util::vector_contains(B, rootPath)) {
                B.emplace_back(std::move(rootPath)); // now total path
            }

            // Add back the edges and nodes that were removed from the graph.
            weight_map.clear();
            spurNode = hop.destination;
            ++i;
        }
        if(B.empty()) {
            // This handles the case of there being no spur paths, or no spur paths left.
            // This could happen if the spur paths have already been exhausted (added to A),
            // or there are no spur paths at all - such as when both the source and sink vertices
            // lie along a "dead end".
            break;
        }

        // Sort the potential k-shortest paths by cost.
        std::ranges::sort(B, [](const auto& lhs, const auto& rhs) { return lhs.size() < rhs.size(); });

        // Add the lowest cost path becomes the k-shortest path.
        A.emplace_back(std::move(B.front()));
        B.erase(B.begin());
    }


    return A;
}

auto routing::KShortest::effectivelyRemoveNode(const common::NetworkNodeID node,
                                               const MultiLayeredGraph& graph,
                                               WeightMap& weightMap) const -> void
{
    for(auto& out_link : graph.getEgressQueuesOf(node)) {
        weightMap[out_link.id] = LARGE_NUMBER;
    }
}
