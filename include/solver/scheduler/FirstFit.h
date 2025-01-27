#pragma once

#include "AbstractScheduler.h"

class FirstFit final : public solver::AbstractScheduler
{
public:
    explicit FirstFit(MultiLayeredGraph& graph);

    auto solve(const MultiLayeredGraph& graph,
               const robin_hood::unordered_set<common::FlowNodeID>& active_f,
               const robin_hood::unordered_set<common::FlowNodeID>& req_f,
               common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet override;
    auto name() -> std::string override;

private:
    /**
     * tries to add the flows of search_f to result_set in a first fit manner
     * @param search_f
     * @param utilization
     * @param result_set
     */
    auto add_flows(const robin_hood::unordered_set<common::FlowNodeID>& search_f,
                   common::NetworkUtilizationList& utilization,
                   solver::solutionSet& result_set)
        -> void;

    MultiLayeredGraph& graph_;
};
