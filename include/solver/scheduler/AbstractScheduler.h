#pragma once

#include "graph/MultiLayeredGraph.h"
#include "solver/UtilizationList.h"
#include <any>

namespace solver {

using solutionSet = std::vector<std::pair<common::FlowNodeID, common::ConfigurationNodeID>>;

class AbstractScheduler
{
public:
    virtual auto initialize(std::any param) -> void {
        // can be overwritten in subclasses if needed
    }

    [[nodiscard]] virtual auto solve(const MultiLayeredGraph& graph,
                                     const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                                     const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                                     common::NetworkUtilizationList& network_utilization)
        -> solutionSet = 0;

    [[nodiscard]] virtual auto name() -> std::string = 0;

    virtual ~AbstractScheduler() = default;
};

} // namespace solver
