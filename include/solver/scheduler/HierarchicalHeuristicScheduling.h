#pragma once
#include "AbstractScheduler.h"
#include "fmt/core.h"
#include "solver/ConfigurationRating/ConfigurationRatingFactory.h"
#include "solver/FlowSorting/FlowSorterFactory.h"
#include "solver/Placement.h"
#include "solver/UtilizationList.h"

class HierarchicalHeuristicScheduling final : public solver::AbstractScheduler
{
public:
    HierarchicalHeuristicScheduling(MultiLayeredGraph& graph,
                                    flow_sorting::FlowSorterTypes sorter_type,
                                    configuration_rating::ConfigurationRatingTypes rating_type,
                                    placement::ConfigPlacementTypes placement_type);

    auto solve(const MultiLayeredGraph& graph,
               const robin_hood::unordered_set<common::FlowNodeID>& active_f,
               const robin_hood::unordered_set<common::FlowNodeID>& req_f,
               common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet override;

    auto name() -> std::string override
    {
        return fmt::format("H2S-{}-{}", to_int(flow_sorter_type_), to_int(config_rating_type_));
    };

private:
    [[nodiscard]] auto scheduleSet(const robin_hood::unordered_set<common::FlowNodeID>& flow_set, common::NetworkUtilizationList& utilization) -> solver::solutionSet;


    MultiLayeredGraph& graph_;
    flow_sorting::FlowSorterTypes flow_sorter_type_;
    configuration_rating::ConfigurationRatingTypes config_rating_type_;
    placement::ConfigPlacementTypes placement_strategy_;
};
