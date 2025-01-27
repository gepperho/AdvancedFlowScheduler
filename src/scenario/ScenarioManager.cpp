#include "scenario/ScenarioManager.h"
#include "routing/DijkstraOverlap.h"
#include "util/ScheduleVerifier.h"
#include <IO/InputParser.h>
#include <IO/OutputLogger.h>
#include <graph/GraphStructOperations.h>
#include <util/Timer.h>

auto ScenarioManager::runScenario(const ProgramOptions& options,
                                  MultiLayeredGraph& graph,
                                  std::unique_ptr<solver::AbstractScheduler> solver,
                                  std::shared_ptr<routing::AbstractNavigation> navigator)
    -> void
{
    /*
     * iterate time steps
     *  clear removed slots
     *  perform offensive and defensive planning
     *  store results and reserved time slots
     *  measure all the stuff
     */
    auto& result_logger = io::OutputLogger::getInstance();
    result_logger.setStrategy(solver->name());
    result_logger.setRouting(navigator->name());

    // load scenario file
    auto scenario = io::parseScenario(options.getScenarioPath());

    // get the max hyper cycle of the scenario
    auto hyper_cycle = util::calculate_hyper_cycle(scenario);

    const auto sub_cycle = util::calculate_gcd_period(scenario); // util::calculate_min_period(scenario);

    currently_active_utilization_ = common::NetworkUtilizationList(graph.getNumberOfEgressQueues(), hyper_cycle, sub_cycle);
    for(auto& time_step : scenario) {
        auto [req_f, pre_configuration_time] = handleFlowChanges(graph, time_step, navigator, options.getCandidatePaths());

        // defensive planning
        // ==================
        auto defensive_solving_timer = Timer();

        solver->initialize(graph);

        auto defensive_solution_set = solver->solve(graph, {}, req_f, currently_active_utilization_);
        auto defensive_solve_time = defensive_solving_timer.elapsed();

        auto scheduling_table_sizes = util::calculate_scheduling_table_sizes(currently_active_utilization_);
        auto defensive_log_wrapper = io::MetaDataLog{.flows_scheduled = defensive_solution_set.size() + active_f_.size(),
                                                     .flows_total = graph.getNumberOfFlows(),
                                                     .config_time = pre_configuration_time,
                                                     .solving_time = defensive_solve_time,
                                                     .time_step = time_step.time,
                                                     .planning_mode = "defensive",
                                                     .traffic = util::calculate_ingress_traffic(active_f_, graph) + util::calculate_ingress_traffic(defensive_solution_set, graph),
                                                     .number_of_frames = util::calculate_number_of_frames(defensive_solution_set, graph) + util::calculate_number_of_frames(active_f_, graph),
                                                     .max_queue_size = util::calculate_max_queue_size(currently_active_utilization_, graph),
                                                     .avg_scheduling_table_size = util::get_average_value(scheduling_table_sizes),
                                                     .max_scheduling_table_size = *std::ranges::max_element(scheduling_table_sizes)};

        auto defensive_post_processing = defensive_solving_timer.elapsed() - defensive_solve_time;


        // offensive planning
        // ==================
        auto offensive_solving_timer = Timer();
        solver::solutionSet offensive_solution_set;

        common::NetworkUtilizationList offensive_utilization(graph.getNumberOfEgressQueues(), hyper_cycle, sub_cycle);

        auto offensive_required = defensive_solution_set.size() < req_f.size();
        if(offensive_required and options.isUseOffensivePlanning()) {
            // skip offensive if defensive has already scheduled all flow's
            offensive_solution_set = solver->solve(graph, active_f_, req_f, offensive_utilization);
        }

        auto offensive_solve_time = offensive_solving_timer.elapsed();

        scheduling_table_sizes = util::calculate_scheduling_table_sizes(offensive_utilization);

        auto offensive_log_wrapper = io::MetaDataLog{
            .flows_scheduled = offensive_solution_set.size(),
            .flows_total = graph.getNumberOfFlows(),
            .config_time = pre_configuration_time,
            .solving_time = offensive_solve_time,
            .time_step = time_step.time,
            .planning_mode = offensive_required and options.isUseOffensivePlanning() ? "offensive" : "skipped",
            .traffic = util::calculate_ingress_traffic(offensive_solution_set, graph),
            .number_of_frames = util::calculate_number_of_frames(offensive_solution_set, graph),
            .max_queue_size = util::calculate_max_queue_size(offensive_utilization, graph),
            .avg_scheduling_table_size = util::get_average_value(scheduling_table_sizes),
            .max_scheduling_table_size = *std::ranges::max_element(scheduling_table_sizes)};

        auto offensive_post_processing_time = offensive_solving_timer.elapsed() - offensive_solve_time;

        auto post_configuration_timer = Timer();

        // decide for a solution (defensive or offensive)
        bool use_defensive_solution = defensive_solution_set.size() == req_f.size()
            or defensive_log_wrapper.traffic >= offensive_log_wrapper.traffic;
        if(use_defensive_solution) {
            for(const auto& flowID : defensive_solution_set | std::views::keys) {
                active_f_.insert(flowID);
            }
        } else {
            currently_active_utilization_ = offensive_utilization;
            for(const auto& flowID : offensive_solution_set | std::views::keys) {
                active_f_.insert(flowID);
            }
        }

        scheduling_table_sizes = util::calculate_scheduling_table_sizes(currently_active_utilization_);
        auto aggregated_log_wrapper = io::MetaDataLog{
            .flows_scheduled = active_f_.size(),
            .flows_total = graph.getNumberOfFlows(),
            .config_time = pre_configuration_time,
            .solving_time = defensive_solve_time + offensive_solve_time,
            .time_step = time_step.time,
            .planning_mode = "aggregated",
            .traffic = use_defensive_solution ? defensive_log_wrapper.traffic : offensive_log_wrapper.traffic,
            .number_of_frames = use_defensive_solution ? defensive_log_wrapper.number_of_frames : offensive_log_wrapper.number_of_frames,
            .max_queue_size = use_defensive_solution ? defensive_log_wrapper.max_queue_size : offensive_log_wrapper.max_queue_size,
            .avg_scheduling_table_size = util::get_average_value(scheduling_table_sizes),
            .max_scheduling_table_size = *std::ranges::max_element(scheduling_table_sizes)};

        if(active_f_.size() < graph.getNumberOfFlows()) {
            // remove rejected flows
            removeRejectedFlows(graph);
        }
        auto post_configuration_time = post_configuration_timer.elapsed();
        defensive_log_wrapper.config_time += post_configuration_time + defensive_post_processing;
        offensive_log_wrapper.config_time += post_configuration_time + offensive_post_processing_time;
        aggregated_log_wrapper.config_time += post_configuration_time + defensive_post_processing + offensive_post_processing_time;

        result_logger.addLog(defensive_log_wrapper);
        result_logger.addLog(offensive_log_wrapper);
        result_logger.addLog(aggregated_log_wrapper);

        // Verify the schedule if enabled
        if(options.isVerifySchedule()) {
            verifySchedule(currently_active_utilization_, graph, hyper_cycle);
        }



        /*
         * TODO
         *  if the resulting scheduling should be written to the disk (or some other output), here would be the place to do it.
         */
    }

    /*
     * TODO
     *  Use the following code snippet to write the egress queue utilization to file
     */
    // currently_active_utilization_.printFreeTimeSlots(solver->name());

    currently_active_utilization_.clear();
}

auto ScenarioManager::handleFlowChanges(MultiLayeredGraph& graph, io::TimeStep& time_step,
                                        const std::shared_ptr<routing::AbstractNavigation>& navigator,
                                        const std::size_t no_candidate_paths)
    -> std::pair<robin_hood::unordered_set<common::FlowNodeID>, double>
{
    const auto update_config_timer = Timer();

    // remove flows
    currently_active_utilization_.removeConfigs(time_step.remove_flows);
    std::ranges::for_each(time_step.remove_flows, [&](const auto flow_id) {
        graph.removeFlow(flow_id);
        active_f_.erase(flow_id);
    });

    // add flows
    robin_hood::unordered_set<common::FlowNodeID> req_f;
    std::ranges::for_each(time_step.add_flows, [&](auto& flow) {
        auto routes = navigator->findRoutes(flow.source, flow.destination, graph, no_candidate_paths);
        auto flow_id = flow.id;
        graph.addFlow(std::move(flow));
        req_f.insert(flow_id);

        for(const auto& path : routes) {
            graph_struct_operations::insertConfiguration(graph, flow_id, path);
        }
    });
    auto update_config_time = update_config_timer.elapsed();
    total_insert_config_time_ += update_config_time;
    return std::pair(req_f, update_config_time);
}

auto ScenarioManager::removeRejectedFlows(MultiLayeredGraph& graph) -> void
{
    std::vector<common::FlowNodeID> to_be_removed;
    std::ranges::for_each(graph.getFlows(), [&](auto tuple) {
        if(auto flow_id = tuple.first;
           not active_f_.contains(flow_id)) {
            to_be_removed.emplace_back(flow_id);
        }
    });

    // remove flows
    std::ranges::for_each(to_be_removed, [&](auto flow_id) {
        graph.removeFlow(flow_id);
    });
}
