#include "solver/scheduler/FirstFit.h"
#include "IO/InputParser.h"
#include "routing/DijkstraOverlap.h"

#include <gtest/gtest.h>
#include <testUtil.h>

class FirstFitTest : public testing::Test
{
protected:
    void SetUp() override
    {
        const auto* const network_graph_path = "../../test/test_data/line.txt";
        graph = io::parseNetworkGraph(network_graph_path).value();

        solver = std::make_unique<FirstFit>(graph);
    }

    auto insertTraffic(const std::size_t number_of_flows, const std::size_t period, const std::size_t package_size,
                       const common::NetworkNodeID source, const common::NetworkNodeID destination)
        -> std::vector<common::FlowNodeID>
    {
        std::vector<common::FlowNodeID> inserted_flows;
        inserted_flows.reserve(number_of_flows);
        routing::DijkstraOverlap navigator;

        const auto next_id = graph.getNumberOfFlows();
        for(std::size_t i = next_id; i < next_id + number_of_flows; ++i) {
            auto flow_id = common::FlowNodeID{i};

            auto route = navigator.findRoutes(source, destination, graph, 1);
            auto config_id = graph.insertConfiguration(flow_id, route.front());

            const auto flow = graph_structs::Flow{
                .id = flow_id,
                .frame_size = package_size,
                .period = period,
                .source = source,
                .destination = destination,
                .configurations = {config_id}};
            graph.addFlow(flow);
            inserted_flows.emplace_back(flow_id);
        }

        return inserted_flows;
    }

    auto run_ff(const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                common::NetworkUtilizationList& utilizationList)
        -> solver::solutionSet
    {
        auto result = solver->solve(graph, {}, req_f, utilizationList);
        if(result.size() < req_f.size()) {
            utilizationList.clear();
            result = solver->solve(graph, active_f, req_f, utilizationList);
        }
        check_reservation_overlaps(utilizationList);
        return result;
    }

    MultiLayeredGraph graph;
    std::unique_ptr<solver::AbstractScheduler> solver;
};

TEST_F(FirstFitTest, trivial)
{
    auto step1 = insertTraffic(10, 1000, 500, common::NetworkNodeID{0}, common::NetworkNodeID{3});
    auto step2 = insertTraffic(10, 1000, 500, common::NetworkNodeID{1}, common::NetworkNodeID{2});

    ASSERT_EQ(graph.getNumberOfFlows(), 20);
    ASSERT_EQ(graph.getNumberOfConfigs(), 20);

    robin_hood::unordered_set<common::FlowNodeID> active_f;
    robin_hood::unordered_set<common::FlowNodeID> req_f;

    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 1000, 1000);

    std::ranges::for_each(step1, [&](auto f) { active_f.insert(f); });
    std::ranges::for_each(step2, [&](auto f) { req_f.insert(f); });

    const auto result_1 = run_ff({}, active_f, utilization);
    ASSERT_EQ(result_1.size(), 10);

    const auto result_2 = run_ff(active_f, req_f, utilization);
    // check against 10, because run_ff yields only the new streams if all of them can be added.
    ASSERT_EQ(result_2.size(), 10);
}

TEST_F(FirstFitTest, congestion)
{
    common::NetworkUtilizationList utilization(graph.getNumberOfEgressQueues(), 500, 500);

    constexpr auto number_of_congestion_load_flows = std::size_t{80};

    auto step1 = insertTraffic(number_of_congestion_load_flows, 500, 625, common::NetworkNodeID{0}, common::NetworkNodeID{3});

    auto step2a = insertTraffic(15, 500, 1500, common::NetworkNodeID{1}, common::NetworkNodeID{2});
    auto step2b = insertTraffic(20, 500, 125, common::NetworkNodeID{1}, common::NetworkNodeID{2});

    robin_hood::unordered_set<common::FlowNodeID> active_f;
    robin_hood::unordered_set<common::FlowNodeID> req_f;
    std::ranges::for_each(step1, [&](auto f) { active_f.insert(f); });

    // create initial load
    const auto congestion_load = run_ff({}, active_f, utilization);
    ASSERT_EQ(congestion_load.size(), number_of_congestion_load_flows);

    std::ranges::for_each(step2a, [&](auto f) { req_f.insert(f); });
    std::ranges::for_each(step2b, [&](auto f) { req_f.insert(f); });

    const auto result = run_ff(active_f, req_f, utilization);
    ASSERT_EQ(result.size(), number_of_congestion_load_flows + 5 + 2);
}