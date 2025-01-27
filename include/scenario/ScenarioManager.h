#pragma once

#include "IO/ProgramOptions.h"
#include "solver/scheduler/AbstractScheduler.h"
#include <graph/MultiLayeredGraph.h>
#include <routing/AbstractNavigation.h>
#include <scenario/TimeStep.h>

class ScenarioManager
{
public:
    auto runScenario(const ProgramOptions& options,
                     MultiLayeredGraph& graph,
                     std::unique_ptr<solver::AbstractScheduler> solver,
                     std::shared_ptr<routing::AbstractNavigation> navigator)
        -> void;

private:
    /**
     * removes flows from graph that are removed in time_step and adds flows accordingly.
     * @param graph
     * @param time_step
     * @param navigator
     * @param no_candidate_paths
     * @return pair, first element: reqF flow set, second element: time required for the flow handling
     */
    auto handleFlowChanges(MultiLayeredGraph& graph,
                           io::TimeStep& time_step,
                           const std::shared_ptr<routing::AbstractNavigation> &navigator,
                           std::size_t no_candidate_paths)
        -> std::pair<robin_hood::unordered_set<common::FlowNodeID>, double>;

    auto removeRejectedFlows(MultiLayeredGraph& graph) -> void;

    double total_insert_config_time_ = 0.;
    robin_hood::unordered_set<common::FlowNodeID> active_f_;
    common::NetworkUtilizationList currently_active_utilization_;
};
