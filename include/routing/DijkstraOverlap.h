#pragma once

#include "AbstractNavigation.h"

namespace routing {

class DijkstraOverlap final : public AbstractNavigation
{

public:
    auto name() -> std::string override;

    /**
     * calculates routes between source and destination node.
     * Uses a Dijkstra that increases the edge weights of all edges used by a path, to encourage using different edges next time.
     * @param source
     * @param destination
     * @param network
     * @param number_of_candidates number of candidates to be calculated. If there are not as many distinct paths, less paths will be returned.
     * @return
     */
    [[nodiscard]] auto findRoutes(common::NetworkNodeID source, common::NetworkNodeID destination,
                                  const MultiLayeredGraph &network,
                                  std::size_t number_of_candidates)
        -> std::vector<std::vector<common::NetworkQueueID>> override;

private:
    const int DUPLICATE_PATH_LIMIT = 10;
};
} // namespace routing