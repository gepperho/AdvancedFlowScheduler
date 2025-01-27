#pragma once

#include "AbstractScheduler.h"
#include <optional>

class EarliestDeadlineFirst final : public solver::AbstractScheduler
{
public:
    explicit EarliestDeadlineFirst(MultiLayeredGraph& graph);

    /**
     * Note that EDF does not use the NetworkUtilizationList and does not support defensive planning.
     * @param graph
     * @param active_f
     * @param req_f
     * @param network_utilization
     * @return
     */
    auto solve(const MultiLayeredGraph& graph,
               const robin_hood::unordered_set<common::FlowNodeID>& active_f,
               const robin_hood::unordered_set<common::FlowNodeID>& req_f,
               common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet override;

    auto name() -> std::string override
    {
        return "EDF";
    }

private:
    [[nodiscard]] auto simulateEdfPlacement(const solver::solutionSet& configs,
                                            common::NetworkUtilizationList& utilizationList) const
        -> std::optional<common::FlowNodeID>;

    MultiLayeredGraph& graph_;
    std::size_t hyper_cycle_;
    bool skip_run_ = true;
};
