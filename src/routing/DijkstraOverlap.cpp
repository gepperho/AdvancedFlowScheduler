#include "routing/DijkstraOverlap.h"
#include "routing/RoutingUtil.h"
#include "util/UtilFunctions.h"

namespace routing {

auto DijkstraOverlap::name() -> std::string
{
    return "DijkstraOverlap";
}


auto DijkstraOverlap::findRoutes(const common::NetworkNodeID source, const common::NetworkNodeID destination,
                                 const MultiLayeredGraph &network,
                                 const std::size_t number_of_candidates)
    -> std::vector<std::vector<common::NetworkQueueID>>
{
    // collect different paths
    WeightMap edge_weights;
    std::vector<std::vector<common::NetworkQueueID>> candidate_paths;

    auto duplicate_counter = 0;
    while(candidate_paths.size() < number_of_candidates and duplicate_counter < DUPLICATE_PATH_LIMIT) {
        auto temp_path = getDijkstraShortestPath(source, destination, network, edge_weights);
        for(const auto &hop : temp_path) {
            // modify weight
            int64_t weight = 1;
            if(edge_weights.contains(hop)) {
                weight = edge_weights[hop];
            }
            weight += 2;
            edge_weights[hop] = weight;
        }

        if(util::vector_contains(candidate_paths, temp_path)) {
            ++duplicate_counter;
        } else {
            candidate_paths.emplace_back(temp_path);
        }
    }
    return candidate_paths;
}

} // namespace routing