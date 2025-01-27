#pragma once

#include "solver/Placement.h"
#include <solver/FlowSorting/FlowSorterFactory.h>
#include <string>


class ProgramOptions
{
public:
    ProgramOptions(int argc, char* argv[]);

    auto getNetworkPath() const -> const std::string&;
    auto isRaw() const -> bool;
    auto getScenarioPath() const -> const std::string&;
    auto getConfigurationRating() const -> std::size_t;
    auto getFlowSorting() const -> flow_sorting::FlowSorterTypes;
    auto getAlgorithm() const -> const std::string&;
    auto getRoutingAlgorithm() const -> const std::string&;
    auto isUseOffensivePlanning() const -> bool;
    auto getPlacementType() const -> placement::ConfigPlacementTypes;
    auto getCandidatePaths() const -> std::size_t;
    auto isVerifySchedule() const -> bool;

private:
    std::string network_path_;
    std::string scenario_path_;
    bool raw_ = false;
    std::string algorithm_ = "H2S";
    std::string routing_algorithm_ = "DIJKSTRA_OVERLAP";
    std::size_t configuration_rating_ = 1;
    std::size_t candidate_paths_ = 5;
    flow_sorting::FlowSorterTypes flow_sorting_ = flow_sorting::FlowSorterTypes::LOW_PERIOD_FLOWS_FIRST;
    bool use_offensive_planning_ = false;
    placement::ConfigPlacementTypes placement_type_ = placement::ConfigPlacementTypes::BALANCED;
    bool verify_schedule = false;
};