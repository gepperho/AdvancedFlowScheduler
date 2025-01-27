#include "solver/ConfigurationRating/CelfRatingFactory.h"
#include "util/UtilFunctions.h"
#include <CLI/CLI.hpp>
#include <IO/ProgramOptions.h>

ProgramOptions::ProgramOptions(const int argc, char **argv)
{
    CLI::App app{"Advanced Flow Scheduler"};

    app.add_option("-n,--network", network_path_,
                   "File path to the network graph as edge list")
        ->required();

    app.add_option("-s,--scenario", scenario_path_,
                   "File path to the scenario as json")
        ->required();

    app.add_flag("-r,--print-raw", raw_,
                 "if set, the results will be printed non pretty for machine parsing");

    app.add_option("-a,--algorithm", algorithm_,
                   "The algorithm/strategy to be used (H2S, CELF, EDF, FF): str [default: H2S]");

    app.add_option("--routing", routing_algorithm_,
                   "The routing algorithm to be used (DIJKSTRA_OVERLAP, K_SHORTEST): str [default: DIJKSTRA_OVERLAP]");

    app.add_option("-c,--configuration-rating", configuration_rating_,
                   "Select the configuration rating heuristic: int [Default: 1]");

    app.add_option("-f,--flow-sorting", flow_sorting_,
                   "Select the flow sorting heuristic: int [Default: 4]");

    app.add_flag("-o, --offensive-planning", use_offensive_planning_,
                 "if set, the offensive planning will be executed when defensive can not schedule all flows.");

    app.add_option("-p, --configuration-placement", placement_type_,
                   "Specifies the configuration placement for H2S and CELF (ASAP: 0, BALANCED: 1): int [default: 0]");

    app.add_option("--candidate-paths", candidate_paths_,
                   "Number of candidate paths to be considered for routing: int [default: 5]. Some algorithms might overwrite this value.");

    app.add_flag("--verify-schedule", verify_schedule,
                 "if set, the schedule will be double checked after the scheduling. This flag is for development.");

    try {
        app.parse(argc, argv);
    } catch(const CLI::ParseError &e) {
        std::exit(app.exit(e));
    }

    if(util::to_lower(algorithm_) == "hermes") {
        candidate_paths_ = 1;
    } else if(util::to_lower(algorithm_) == "celf" and configuration_rating_ == to_int(celf_rating::CelfRatingTypes::LowPeriodLongPaths)) {
        // greedy algorithm from "Optimization algorithms for the scheduling of IEEE 802.1 Time-Sensitive Networking (TSN)"
        candidate_paths_ = 1;
    }
}
auto ProgramOptions::getNetworkPath() const -> const std::string &
{
    return network_path_;
}
auto ProgramOptions::isRaw() const -> bool
{
    return raw_;
}
auto ProgramOptions::getScenarioPath() const -> const std::string &
{
    return scenario_path_;
}
auto ProgramOptions::getConfigurationRating() const -> std::size_t
{
    return configuration_rating_;
}
auto ProgramOptions::getFlowSorting() const -> flow_sorting::FlowSorterTypes
{
    return flow_sorting_;
}
const std::string &ProgramOptions::getAlgorithm() const
{
    return algorithm_;
}

auto ProgramOptions::getRoutingAlgorithm() const -> const std::string &
{
    return routing_algorithm_;
}

auto ProgramOptions::isUseOffensivePlanning() const -> bool
{
    return use_offensive_planning_;
}
auto ProgramOptions::getPlacementType() const -> placement::ConfigPlacementTypes
{
    return placement_type_;
}

auto ProgramOptions::getCandidatePaths() const -> std::size_t
{
    return candidate_paths_;
}
auto ProgramOptions::isVerifySchedule() const -> bool
{
    return verify_schedule;
}