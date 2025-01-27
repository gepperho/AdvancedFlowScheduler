#include "routing/DijkstraOverlap.h"
#include "routing/KShortest.h"
#include "solver/scheduler/CelfFlowQueuing.h"
#include "solver/scheduler/EarliestDeadlineFirst.h"
#include "solver/scheduler/FirstFit.h"
#include "solver/scheduler/Hermes.h"
#include "solver/scheduler/HierarchicalHeuristicScheduling.h"
#include <IO/InputParser.h>
#include <IO/OutputLogger.h>
#include <scenario/ScenarioManager.h>
#include <util/Timer.h>

auto main(int argc, char* argv[])
    -> int
{
    auto options = ProgramOptions(argc, argv);

    if(not options.isRaw()) {
        fmt::print("Start Advanced Flow Scheduler\n");
        fmt::print("Network path: {}\nScenario path: {}\n", options.getNetworkPath(), options.getScenarioPath());
    }
    auto& result_logger = io::OutputLogger::getInstance();
    result_logger.setNetwork(options.getNetworkPath());
    result_logger.setScenario(options.getScenarioPath());
    result_logger.setPlacement(options.getPlacementType());
    result_logger.setNoCandidateRoutes(options.getCandidatePaths());

    auto parse_network_timer = Timer();
    auto graph = io::parseNetworkGraph(options.getNetworkPath()).value();
    auto parse_network_time = parse_network_timer.elapsed();

    if(not options.isRaw()) {
        fmt::print("Network read after {}s\n", parse_network_time);
    }

    // =============
    // Create Solver
    // =============
    std::unique_ptr<solver::AbstractScheduler> solver;
    const std::string algorithm_name = util::to_upper(options.getAlgorithm());
    if(algorithm_name == "CELF") {
        solver = std::make_unique<CelfFlowQueuing>(
            graph,
            static_cast<celf_rating::CelfRatingTypes>(options.getConfigurationRating()),
            options.getPlacementType());
    } else if(algorithm_name == "EDF") {
        solver = std::make_unique<EarliestDeadlineFirst>(graph);
    } else if(algorithm_name == "FF" or algorithm_name == "FIRSTFIT") {
        solver = std::make_unique<FirstFit>(graph);
    } else if(algorithm_name == "HERMES") {
        solver = std::make_unique<Hermes>(graph);
    } else {
        solver = std::make_unique<HierarchicalHeuristicScheduling>(
            graph,
            options.getFlowSorting(),
            static_cast<configuration_rating::ConfigurationRatingTypes>(options.getConfigurationRating()),
            options.getPlacementType());
    }

    // ================
    // Create Navigator
    // ================
    std::shared_ptr<routing::AbstractNavigation> navigator;
    const std::string routing_algorithm = util::to_upper(options.getRoutingAlgorithm());
    if(routing_algorithm == "K_SHORTEST") {
        navigator = std::make_shared<routing::KShortest>();
    } else {
        navigator = std::make_shared<routing::DijkstraOverlap>();
    }

    // ============
    // Run Scenario
    // ============
    ScenarioManager manager;

    auto total_scenario_timer = Timer();
    manager.runScenario(options, graph, std::move(solver), std::move(navigator));
    auto total_scenario_time = total_scenario_timer.elapsed();

    if(options.isRaw()) {
        result_logger.printRawLog();
    } else {
        result_logger.printLog();
        fmt::print("total scenario runtime: {}s\n", total_scenario_time);
    }
}
