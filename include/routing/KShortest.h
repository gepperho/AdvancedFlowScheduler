#pragma once

#include "RoutingUtil.h"
#include <routing/AbstractNavigation.h>

namespace routing {

class KShortest final : public routing::AbstractNavigation
{
public:
    auto name() -> std::string override;
    auto findRoutes(common::NetworkNodeID source,
                    common::NetworkNodeID destination,
                    const MultiLayeredGraph& network,
                    std::size_t number_of_candidates)
        -> std::vector<std::vector<common::NetworkQueueID>> override;

private:
    auto effectivelyRemoveNode(common::NetworkNodeID node,
                               const MultiLayeredGraph& graph,
                               routing::WeightMap& weightMap) const -> void;

    const std::size_t LARGE_NUMBER = 1000000;
};


} // namespace routing