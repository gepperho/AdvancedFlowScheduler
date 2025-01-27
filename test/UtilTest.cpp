#include "IO/InputParser.h"
#include "routing/DijkstraOverlap.h"
#include "scenario/TimeStep.h"
#include "solver/scheduler/HierarchicalHeuristicScheduling.h"

#include <graph/MultiLayeredGraph.h>
#include <gtest/gtest.h>
#include <testUtil.h>
#include <util/UtilFunctions.h>


inline auto create_dummy_graph_with_flows(const std::vector<std::size_t>& vec) -> MultiLayeredGraph
{
    const auto* const network_graph_path = "../../test/test_data/line.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    routing::DijkstraOverlap navigator;

    std::size_t counter = 0;
    for(const auto period : vec) {
        graph_structs::Flow f{.id = common::FlowNodeID{counter},
                              .frame_size = 125,
                              .period = period,
                              .source = common::NetworkNodeID{0},
                              .destination = common::NetworkNodeID{3}};
        auto route = navigator.findRoutes(f.source, f.destination, graph, 1);
        auto config_id = graph.insertConfiguration(f.id, route.front());
        f.configurations = {config_id};
        graph.addFlow(f);
        ++counter;
    }
    return graph;
}

inline auto create_dummy_scenario_with_flows(const std::vector<std::size_t>& vec) -> std::vector<io::TimeStep>
{
    std::vector<io::TimeStep> scenario;

    std::size_t counter = 0;
    io::TimeStep current_step;
    current_step.time = 0;
    for(const auto period : vec) {
        if(counter % 4 == 0 and counter > 3) {
            scenario.emplace_back(current_step);
            current_step = io::TimeStep();
            current_step.time = counter / 4;
        }
        current_step.add_flows.emplace_back(graph_structs::Flow{.id = common::FlowNodeID{counter}, .period = period});

        ++counter;
    }
    scenario.emplace_back(current_step);

    return scenario;
}


TEST(UtilTest, HypercycleCalc)
{
    const std::vector<std::size_t> input1{10, 15, 25, 10};
    const auto graph = create_dummy_graph_with_flows(input1);
    const auto hyper_cycle_graph = util::calculate_hyper_cycle(graph);

    EXPECT_EQ(hyper_cycle_graph, 150);

    const auto scenario = create_dummy_scenario_with_flows(input1);
    const auto hyper_cycle_scenario = util::calculate_hyper_cycle(scenario);
    EXPECT_EQ(hyper_cycle_graph, hyper_cycle_scenario);
}

TEST(UtilTest, HypercycleCalc2)
{
    std::vector<std::size_t> input1{250, 500, 1000, 125};

    // just make it very large with the same numbers
    for(int i = 0; i < 10000; ++i) {
        input1.emplace_back(input1[i % 4]);
    }

    const auto graph = create_dummy_graph_with_flows(input1);
    const auto hyper_cycle_graph = util::calculate_hyper_cycle(graph);

    EXPECT_EQ(hyper_cycle_graph, 1000);

    const auto scenario = create_dummy_scenario_with_flows(input1);
    const auto hyper_cycle_scenario = util::calculate_hyper_cycle(scenario);
    EXPECT_EQ(hyper_cycle_graph, hyper_cycle_scenario);
}

TEST(UtilTest, HypercylceIncorrectInput)
{
    const std::vector<std::size_t> input1{10, 15, 25, 0};
    const auto graph = create_dummy_graph_with_flows(input1);
    const auto hyper_cycle_graph = util::calculate_hyper_cycle(graph);

    ASSERT_EQ(hyper_cycle_graph, 0);

    const auto scenario = create_dummy_scenario_with_flows(input1);
    const auto hyper_cycle_scenario = util::calculate_hyper_cycle(scenario);
    EXPECT_EQ(hyper_cycle_graph, hyper_cycle_scenario);
}

TEST(UtilTest, gcd_period)
{
    const std::vector<std::size_t> input1{10, 15, 25, 1000, 10, 25};
    const auto scenario = create_dummy_scenario_with_flows(input1);
    EXPECT_EQ(5, util::calculate_gcd_period(scenario));

    const std::vector<std::size_t> input2{300, 500, 700, 1100, 1300, 1700};
    const auto scenario2 = create_dummy_scenario_with_flows(input2);
    EXPECT_EQ(100, util::calculate_gcd_period(scenario2));

    const std::vector<std::size_t> input3{500, 1250, 750, 2000, 2500};
    const auto scenario3 = create_dummy_scenario_with_flows(input3);
    EXPECT_EQ(250, util::calculate_gcd_period(scenario3));
}

TEST(UtilTest, calculate_transmission_delay)
{
    for(std::size_t i = 1; i <= 12; ++i) {
        ASSERT_EQ(i, util::calculate_transmission_delay(125 * i));
    }
}

TEST(UtilTest, resultMetricsNoFlows)
{
    // Create an empty graph and utilization list
    const auto graph = create_dummy_graph_with_flows({});
    const auto scenario = create_dummy_scenario_with_flows({});
    const common::NetworkUtilizationList utilizationList(graph.getNumberOfEgressQueues(),
                                                         util::calculate_hyper_cycle(graph),
                                                         util::calculate_gcd_period(scenario));

    // Call the function being tested
    const auto linkUtilization = util::calculateLinkUtilization(graph, utilizationList);

    // Assert that the result is an empty unordered map
    for(const auto& pair : linkUtilization) {
        auto [id, value] = pair;
        EXPECT_EQ(0, value);
    }
    EXPECT_FLOAT_EQ(0.f, util::calculateAverageLinkUtilization(linkUtilization));
}

TEST(UtilTest, resultMetricsWithFlows)
{
    // Create a graph and utilization list with test data
    std::vector<std::size_t> input1{250, 500, 1000};

    // Populate the graph and utilization list with necessary data for the test case
    auto graph = create_dummy_graph_with_flows(input1);
    auto scenario = create_dummy_scenario_with_flows(input1);

    auto solver = std::make_unique<HierarchicalHeuristicScheduling>(
        graph,
        flow_sorting::FlowSorterTypes::LOW_PERIOD_FLOWS_FIRST,
        configuration_rating::ConfigurationRatingTypes::PATH_LENGTH,
        placement::ConfigPlacementTypes::BALANCED);

    common::NetworkUtilizationList utilizationList(graph.getNumberOfEgressQueues(),
                                                   util::calculate_hyper_cycle(graph),
                                                   util::calculate_gcd_period(scenario));
    solver->initialize(graph);
    robin_hood::unordered_set<common::FlowNodeID> reqF;
    for(const auto& flow : scenario.front().add_flows) {
        reqF.insert(flow.id);
    }
    auto solutionSet = solver->solve(graph, {}, reqF, utilizationList);
    EXPECT_EQ(std::size_t{3}, solutionSet.size());

    // Call the function being tested
    auto result = util::calculateLinkUtilization(graph, utilizationList);

    EXPECT_FALSE(result.empty());
    std::vector<std::size_t> expected_utilization_values = {7, 0, 0, 0, 0, 7, 0, 0, 7, 0, 0, 7, 7, 0};

    EXPECT_EQ(result.size(), expected_utilization_values.size());

    for(std::size_t i = 0; i < result.size(); ++i) {
        EXPECT_EQ(expected_utilization_values.at(i),
                  result.at(common::NetworkQueueID{i}));
    }

    // average link util
    auto expected_average = 7.f * 5.f / static_cast<float>(expected_utilization_values.size());
    auto actual_average = util::calculateAverageLinkUtilization(result);
    EXPECT_FLOAT_EQ(expected_average, actual_average);

    // aggregated network throughput
    auto expected_traffic = 7.f * 125.f * 8.f / 1000.f;
    auto actual_traffic = util::calculate_ingress_traffic(solutionSet, graph);
    EXPECT_FLOAT_EQ(expected_traffic, actual_traffic);

    // number of frames
    auto expected_frames = 7;
    auto actual_frames = util::calculate_number_of_frames(solutionSet, graph);
    EXPECT_EQ(expected_frames, actual_frames);
}

TEST(UtilTest, MaxQueueSize)
{
    std::vector<std::size_t> periods{1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000};
    std::vector<std::size_t> frame_sizes{500, 1500, 250, 750, 125, 375, 125, 125};
    ASSERT_EQ(periods.size(), frame_sizes.size());

    /*
     * create a graph, similar to the other tests, but have two possible source nodes
     */
    const auto* const network_graph_path = "../../test/test_data/line.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    routing::DijkstraOverlap navigator;
    for(std::size_t i = 0; i < periods.size(); ++i) {
        graph_structs::Flow f{.id = common::FlowNodeID{i},
                              .frame_size = frame_sizes.at(i),
                              .period = periods.at(i),
                              .source = common::NetworkNodeID{0 + i % 2},
                              .destination = common::NetworkNodeID{3}};
        auto route = navigator.findRoutes(f.source, f.destination, graph, 1);
        auto config_id = graph.insertConfiguration(f.id, route.front());
        f.configurations = {config_id};
        graph.addFlow(f);
    }

    // Populate the graph and utilization list with necessary data for the test case
    auto scenario = create_dummy_scenario_with_flows(periods);

    auto solver = std::make_unique<HierarchicalHeuristicScheduling>(
        graph,
        flow_sorting::FlowSorterTypes::LOW_PERIOD_FLOWS_FIRST,
        configuration_rating::ConfigurationRatingTypes::PATH_LENGTH,
        placement::ConfigPlacementTypes::BALANCED);

    common::NetworkUtilizationList utilizationList(graph.getNumberOfEgressQueues(),
                                                   util::calculate_hyper_cycle(graph),
                                                   util::calculate_gcd_period(scenario));
    solver->initialize(graph);

    std::vector<std::size_t> expected_no_flows_scheduled = {4, 8};
    std::vector<std::size_t> expected_max_queue_sizes = {3, 7};
    solver::solutionSet solutionSet;

    for(const auto& time_step : scenario) {
        robin_hood::unordered_set<common::FlowNodeID> reqF;
        for(const auto& flow : time_step.add_flows) {
            reqF.insert(flow.id);
        }
        robin_hood::unordered_set<common::FlowNodeID> activeF;
        for(const auto& flow : solutionSet | std::views::keys) {
            reqF.insert(flow);
        }
        solutionSet = solver->solve(graph, activeF, reqF, utilizationList);
        EXPECT_EQ(expected_no_flows_scheduled.at(time_step.time), solutionSet.size());

        // check for valid slot selections
        check_reservation_overlaps(utilizationList);

        // check the queue sizes
        auto actual_max_size = util::calculate_max_queue_size(utilizationList, graph);
        EXPECT_EQ(expected_max_queue_sizes.at(time_step.time), actual_max_size);
    }
}
