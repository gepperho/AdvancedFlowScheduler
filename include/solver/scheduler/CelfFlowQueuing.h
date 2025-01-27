#pragma once

#include "AbstractScheduler.h"
#include "fmt/core.h"
#include "solver/ConfigurationRating/CelfRatingFactory.h"
#include "solver/FlowSorting/FlowSorterFactory.h"
#include "solver/Placement.h"


class CelfFlowQueuing final : public solver::AbstractScheduler
{
public:
    CelfFlowQueuing(MultiLayeredGraph& graph,
                    celf_rating::CelfRatingTypes rating_type,
                    placement::ConfigPlacementTypes placement_type);

    [[nodiscard]] auto solve(const MultiLayeredGraph& graph,
                             const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                             const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                             common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet override;

    [[nodiscard]] auto name() -> std::string override
    {
        return fmt::format("CelfFlowQueuing-{}", celf_rating::to_int(config_rating_type_));
    };

private:
    [[nodiscard]] auto scheduleSet(const robin_hood::unordered_set<common::FlowNodeID>& set,
                                   common::NetworkUtilizationList& network_utilization) -> solver::solutionSet;

    auto pick_config(const graph_structs::Configuration& config_id, common::NetworkUtilizationList& network_utilization) -> bool;

    MultiLayeredGraph& graph_;
    celf_rating::CelfRatingTypes config_rating_type_;

    robin_hood::unordered_map<common::ConfigurationNodeID, std::pair<float, float>> config_rating_;
    robin_hood::unordered_set<common::ConfigurationNodeID> updated_;
    placement::ConfigPlacementTypes placement_strategy_;
};
