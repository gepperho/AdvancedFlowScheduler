#include "solver/scheduler/Hermes.h"
#include "../testUtil.h"
#include "IO/InputParser.h"
#include "graph/GraphStructOperations.h"
#include "routing/DijkstraOverlap.h"
#include <gtest/gtest.h>


auto insertTraffic(MultiLayeredGraph& graph, const std::size_t number_of_flows, const std::size_t source, const std::size_t destination,
                   const std::size_t period, const std::size_t package_size)
    -> std::vector<common::FlowNodeID>
{
    std::vector<common::FlowNodeID> inserted_flows;
    inserted_flows.reserve(number_of_flows);
    routing::DijkstraOverlap navigator;
    const auto next_id = graph.getNumberOfFlows();
    for(std::size_t i = next_id; i < next_id + number_of_flows; ++i) {
        const auto flow_id = common::FlowNodeID{i};
        const auto sourceID = common::NetworkNodeID{source};
        const auto destinationID = common::NetworkNodeID{destination};

        auto route = navigator.findRoutes(sourceID, destinationID, graph, 1);

        const auto flow = graph_structs::Flow{
            .id = flow_id,
            .frame_size = package_size,
            .period = period,
            .source = sourceID,
            .destination = destinationID};

        inserted_flows.emplace_back(flow_id);
        graph.addFlow(flow);
        graph_struct_operations::insertConfiguration(graph, flow.id, route.front());
    }
    return inserted_flows;
}

auto checkPrecedences(const std::vector<std::vector<std::size_t>>& precedence_chains, const common::NetworkUtilizationList& utilization) -> void
{
    auto precedence = [&utilization](auto low_phase_port_id, auto high_phase_port_id) {
        // high phase network links are traversed earlier (because the phases are assigned in reverse order)
        for(auto& reserved_low : utilization.getReservedEgressSlots().at(low_phase_port_id)) {
            auto [low_start, low_end, low_flow, low_config] = reserved_low;
            for(auto& reserved_high : utilization.getReservedEgressSlots().at(high_phase_port_id)) {
                auto [high_start, high_end, high_flow, high_config] = reserved_high;
                // ensure the high phase transmission has ended when the low phase one starts
                // this ignores processing delay etc.
                if(low_flow.get() == high_flow.get()) {
                    EXPECT_LT(high_end, low_start);
                }
            }
        }
    };
    for(const auto& current_precedence_chain : precedence_chains) {
        for(std::size_t i = 0; i < current_precedence_chain.size() - 1; ++i) {
            precedence(current_precedence_chain.at(i), current_precedence_chain.at(i + 1));
        }
    }
}


TEST(HermesTest, simple)
{
    const auto* const network_graph_path = "../../test/test_data/star.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    auto solver = std::make_unique<Hermes>(graph);

    auto pairwise_flows = std::size_t{1};
    robin_hood::unordered_set<common::FlowNodeID> flows;

    for(int source = 0; source < 4; ++source) {
        for(int destination = 0; destination < 4; ++destination) {
            if(source == destination) {
                continue;
            }
            // 75 incoming/outgoing streams + 8 e2e delay
            auto added = insertTraffic(graph, pairwise_flows, source, destination, 100, 125);
            std::ranges::for_each(added, [&](auto f) { flows.insert(f); });
        }
    }

    auto number_flows_total = pairwise_flows * 4 * 3;

    EXPECT_EQ(graph.getNumberOfFlows(), number_flows_total);
    EXPECT_EQ(graph.getNumberOfConfigs(), number_flows_total);

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 100, 100);
    auto result = solver->solve(graph, {}, flows, utilization);

    ASSERT_EQ(result.size(), number_flows_total);

    // make sure the phases etc. are correct
    std::vector<std::vector<std::size_t>>
        all_precedence_chains = {
            {5, 0},
            {6, 0},
            {7, 0},
            {4, 1},
            {6, 1},
            {7, 1},
            {4, 2},
            {5, 2},
            {7, 2},
            {4, 3},
            {5, 3},
            {6, 3}};

    checkPrecedences(all_precedence_chains, utilization);
    check_reservation_overlaps(utilization);
}


TEST(HermesTest, fully_saturated)
{
    const auto* const network_graph_path = "../../test/test_data/star.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    auto solver = std::make_unique<Hermes>(graph);


    auto pairwise_flows = std::size_t{15};
    robin_hood::unordered_set<common::FlowNodeID> flows;
    std::size_t hyper_period = 96;

    for(int source = 0; source < 4; ++source) {
        for(int destination = 0; destination < 4; ++destination) {
            if(source == destination) {
                continue;
            }
            // 75 incoming/outgoing streams + 8 e2e delay
            auto added = insertTraffic(graph, pairwise_flows, source, destination, hyper_period, 125);
            std::ranges::for_each(added, [&](auto f) { flows.insert(f); });
        }
    }

    auto number_flows_total = pairwise_flows * 4 * 3;

    ASSERT_EQ(graph.getNumberOfFlows(), number_flows_total);
    ASSERT_EQ(graph.getNumberOfConfigs(), number_flows_total);

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), hyper_period, hyper_period);
    auto result = solver->solve(graph, {}, flows, utilization);

    ASSERT_EQ(result.size(), number_flows_total);
    int counter = -1;
    for(const auto& free_slots_of_link : utilization.getFreeEgressSlots()) {
        ++counter;
        if(counter < 4) {
            continue;
        }
        // verify that the last hops free in the first half of the hyper period and almost full in the second half.
        // only the very last tick is free, since it can not be used due to the propagation delay
        auto [first_start, first_last_free] = free_slots_of_link.front();
        auto [second_start, second_last_free] = free_slots_of_link.back();
        EXPECT_EQ(0, first_start);
        EXPECT_EQ(49, first_last_free);
        EXPECT_EQ(95, second_start);
        EXPECT_EQ(95, second_last_free);
        EXPECT_EQ(2, free_slots_of_link.size());
    }

    // make sure the phases etc. are correct
    std::vector<std::vector<std::size_t>>
        all_precedence_chains = {
            {5, 0},
            {6, 0},
            {7, 0},
            {4, 1},
            {6, 1},
            {7, 1},
            {4, 2},
            {5, 2},
            {7, 2},
            {4, 3},
            {5, 3},
            {6, 3}};

    checkPrecedences(all_precedence_chains, utilization);
    check_reservation_overlaps(utilization);
}


TEST(HermesTest, deadlock)
{
    const auto* const network_graph_path = "../../test/test_data/hermes_deadlock.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    const auto solver = std::make_unique<Hermes>(graph);


    constexpr auto no_outgoing_flows = std::size_t{1};
    robin_hood::unordered_set<common::FlowNodeID> flows;

    std::ranges::for_each(insertTraffic(graph, no_outgoing_flows, 11, 12, 100, 125), [&](auto f) { flows.insert(f); });
    std::ranges::for_each(insertTraffic(graph, no_outgoing_flows, 21, 22, 100, 125), [&](auto f) { flows.insert(f); });
    std::ranges::for_each(insertTraffic(graph, no_outgoing_flows, 31, 32, 100, 125), [&](auto f) { flows.insert(f); });
    std::ranges::for_each(insertTraffic(graph, no_outgoing_flows, 41, 42, 100, 125), [&](auto f) { flows.insert(f); });

    constexpr auto number_flows_total = no_outgoing_flows * 4;

    ASSERT_EQ(graph.getNumberOfFlows(), number_flows_total);
    ASSERT_EQ(graph.getNumberOfConfigs(), number_flows_total);

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 100, 100);
    const auto result = solver->solve(graph, {}, flows, utilization);

    ASSERT_EQ(result.size(), 0);
}


TEST(HermesTest, hermesExample)
{
    // example is taken from the hermes paper
    const auto* const network_graph_path = "../../test/test_data/hermes_example.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    auto solver = std::make_unique<Hermes>(graph);

    // create flows
    robin_hood::unordered_set<common::FlowNodeID> flow_ids;
    std::vector flows = {
        graph_structs::Flow{.id = common::FlowNodeID{1}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{6}},
        graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{6}, .destination = common::NetworkNodeID{7}},
        graph_structs::Flow{.id = common::FlowNodeID{3}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{6}, .destination = common::NetworkNodeID{8}},
        graph_structs::Flow{.id = common::FlowNodeID{4}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{7}, .destination = common::NetworkNodeID{9}},
        graph_structs::Flow{.id = common::FlowNodeID{5}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{7}, .destination = common::NetworkNodeID{0}},
        graph_structs::Flow{.id = common::FlowNodeID{6}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{7}, .destination = common::NetworkNodeID{6}},
        graph_structs::Flow{.id = common::FlowNodeID{7}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{8}, .destination = common::NetworkNodeID{0}},
        graph_structs::Flow{.id = common::FlowNodeID{8}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{9}, .destination = common::NetworkNodeID{0}},
        graph_structs::Flow{.id = common::FlowNodeID{9}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{9}, .destination = common::NetworkNodeID{7}}};

    std::vector<std::string> route_strings = {
        "0 1 2 6",
        "6 2 1 3 7",
        "6 2 4 8",
        "7 3 5 9",
        "7 3 1 0",
        "7 3 4 2 6",
        "8 4 2 1 0",
        "9 5 2 1 0",
        "9 5 3 7"};

    routing::DijkstraOverlap navigator;

    print_egress_queue_to_link_mapping(graph);

    for(auto& flow : flows) {
        flow_ids.emplace(flow.id);
        auto routes = navigator.findRoutes(flow.source, flow.destination, graph, 2);
        auto route = routes.front();
        if(flow.id.get() == 6) {
            route = routes.back();
        }

        EXPECT_EQ(route_strings.at(flow.id.get() - 1), create_route_string(route, graph, flow.source));

        graph.addFlow(flow);
        graph_struct_operations::insertConfiguration(graph, flow.id, route);
    }


    // run scheduling

    EXPECT_EQ(graph.getNumberOfFlows(), 9);
    EXPECT_EQ(graph.getNumberOfConfigs(), 9);

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 100, 100);
    auto result = solver->solve(graph, {}, flow_ids, utilization);

    ASSERT_EQ(result.size(), 9);

    // make sure the phases etc. are correct
    std::vector<std::vector<std::size_t>>
        all_precedence_chains = {
            {7, 2, 0},
            {11, 3, 4, 18},
            {14, 5, 18},
            {17, 10, 19},
            {1, 8, 19},
            {7, 12, 9, 19},
            {1, 4, 12, 20},
            {1, 4, 15, 21},
            {11, 16, 21}};

    checkPrecedences(all_precedence_chains, utilization);
    check_reservation_overlaps(utilization);
}

TEST(HermesTest, hermesDeadlockExample)
{
    // example is taken from the hermes paper
    const auto* const network_graph_path = "../../test/test_data/hermes_example.txt";
    auto graph = io::parseNetworkGraph(network_graph_path).value();

    auto solver = std::make_unique<Hermes>(graph);

    // create flows
    robin_hood::unordered_set<common::FlowNodeID> flow_ids;
    std::vector flows = {
        graph_structs::Flow{.id = common::FlowNodeID{1}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{0}, .destination = common::NetworkNodeID{8}},
        graph_structs::Flow{.id = common::FlowNodeID{2}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{7}, .destination = common::NetworkNodeID{0}},
        graph_structs::Flow{.id = common::FlowNodeID{3}, .frame_size = 125, .period = 100, .source = common::NetworkNodeID{8}, .destination = common::NetworkNodeID{7}},
    };
    std::vector<std::string> route_strings = {
        "0 1 3 4 8",
        "7 3 4 2 1 0",
        "8 4 2 1 3 7"};

    routing::DijkstraOverlap navigator;

    print_egress_queue_to_link_mapping(graph);

    for(auto& flow : flows) {
        flow_ids.emplace(flow.id);
        auto routes = navigator.findRoutes(flow.source, flow.destination, graph, 2);
        const auto& route = routes.back();


        EXPECT_EQ(route_strings.at(flow.id.get() - 1), create_route_string(route, graph, flow.source));

        graph.addFlow(flow);
        graph_struct_operations::insertConfiguration(graph, flow.id, route);
    }


    // run scheduling

    ASSERT_EQ(graph.getNumberOfFlows(), flows.size());
    ASSERT_EQ(graph.getNumberOfConfigs(), flows.size());

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 100, 100);
    auto result = solver->solve(graph, {}, flow_ids, utilization);
    check_reservation_overlaps(utilization);

    ASSERT_EQ(result.size(), 0);
}