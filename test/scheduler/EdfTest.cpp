#include "IO/InputParser.h"
#include "routing/DijkstraOverlap.h"
#include "solver/scheduler/EarliestDeadlineFirst.h"

#include <gtest/gtest.h>
#include <testUtil.h>

class EdfTest : public testing::Test
{
protected:
    void SetUp() override
    {
        const auto* const network_graph_path = "../../test/test_data/star.txt";
        graph = io::parseNetworkGraph(network_graph_path).value();

        solver = std::make_unique<EarliestDeadlineFirst>(graph);
    }

    auto insertTraffic(const std::size_t number_of_flows, const std::size_t period, const std::size_t package_size) -> void
    {
        routing::DijkstraOverlap navigator;
        const auto next_id = graph.getNumberOfFlows();
        for(std::size_t i = next_id; i < next_id + number_of_flows; ++i) {
            const auto flow_id = common::FlowNodeID{i};
            constexpr auto source = common::NetworkNodeID{0};
            constexpr auto destination = common::NetworkNodeID{3};

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
        }
    }

    auto run_edf(const std::size_t hyper_cycle) -> solver::solutionSet
    {
        const robin_hood::unordered_set<common::FlowNodeID> active_f;
        robin_hood::unordered_set<common::FlowNodeID> req_f;
        req_f.reserve(graph.getNumberOfFlows());

        auto min_period = std::numeric_limits<std::size_t>::max();
        for(const auto& element : graph.getFlows()) {
            req_f.insert(element.first);
            min_period = std::min(min_period, element.second.period);
        }

        common::NetworkUtilizationList utilizationList(8, hyper_cycle, min_period);

        solver->initialize(0);
        //
        auto skipped_defensive_planning = solver->solve(graph, active_f, req_f, utilizationList);
        auto result = solver->solve(graph, active_f, req_f, utilizationList);
        check_reservation_overlaps(utilizationList);
        return result;
    }

    MultiLayeredGraph graph;
    std::unique_ptr<solver::AbstractScheduler> solver;
};

TEST_F(EdfTest, Infeasible)
{
    constexpr auto number_of_flows = std::size_t{5};

    insertTraffic(number_of_flows, 250, 6250);
    ASSERT_EQ(graph.getNumberOfFlows(), number_of_flows);
    ASSERT_EQ(graph.getNumberOfConfigs(), number_of_flows);

    const auto result = run_edf(250);

    ASSERT_EQ(result.size(), std::size_t{3});
}

TEST_F(EdfTest, Prioritize)
{
    insertTraffic(5, 125, 1250);
    insertTraffic(4, 250, 6250);
    ASSERT_EQ(graph.getNumberOfFlows(), 9);
    ASSERT_EQ(graph.getNumberOfConfigs(), 9);

    const auto result = run_edf(250);

    ASSERT_EQ(result.size(), std::size_t{6});
}

TEST_F(EdfTest, trivial)
{
    constexpr auto number_of_flows = std::size_t{64};
    insertTraffic(number_of_flows, 100, 125);
    insertTraffic(number_of_flows, 200, 125);

    const auto result = run_edf(200);
    ASSERT_EQ(result.size(), std::size_t{2 * number_of_flows});
}

TEST_F(EdfTest, trivial_fail_at_last_hop)
{
    constexpr auto number_of_flows = std::size_t{65};
    insertTraffic(number_of_flows, 100, 125); // order is important, since it will fail the first time and then add them one by one
    insertTraffic(number_of_flows, 200, 125);

    const auto result = run_edf(200);
    // schedules all 65 flows with period 100
    // schedules only 63 flows with period 200
    ASSERT_EQ(result.size(), std::size_t{128});
}