#pragma once

#include "Typedefs.h"
#include "graph/GraphStructs.h"
#include "graph/MultiLayeredGraph.h"

namespace routing {

class AbstractNavigation
{
public:
    [[nodiscard]] virtual auto name() -> std::string = 0;

    /**
     * calculates routes between source and destination node.
     * @param source
     * @param destination
     * @param network
     * @param number_of_candidates number of candidates to be computed. If there are not as many distinct paths, less paths will be returned.
     * @return
     */
    [[nodiscard]] virtual auto findRoutes(common::NetworkNodeID source, common::NetworkNodeID destination,
                                          const MultiLayeredGraph& network,
                                          std::size_t number_of_candidates)
        -> std::vector<std::vector<common::NetworkQueueID>> = 0;

    virtual ~AbstractNavigation() = default;
};
} // namespace routing
