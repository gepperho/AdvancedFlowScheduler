#include "solver/scheduler/HierarchicalHeuristicScheduling.h"
#include "fmt/core.h"
#include "graph/GraphStructOperations.h"
#include "util/UtilFunctions.h"
#include <queue>

HierarchicalHeuristicScheduling::HierarchicalHeuristicScheduling(MultiLayeredGraph& graph,
                                                                 const flow_sorting::FlowSorterTypes sorter_type,
                                                                 const configuration_rating::ConfigurationRatingTypes rating_type,
                                                                 const placement::ConfigPlacementTypes placement_type)
    : graph_(graph),
      flow_sorter_type_(sorter_type),
      config_rating_type_(rating_type),
      placement_strategy_(placement_type)
{}


auto HierarchicalHeuristicScheduling::solve(const MultiLayeredGraph& graph,
                                            const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                                            const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                                            common::NetworkUtilizationList& network_utilization)
    -> solver::solutionSet
{
    graph_ = graph;

    // auto trivial_solution = trivialSolutions();

    solver::solutionSet result;

    if(not active_f.empty()) {
        // schedule active flows if there are any
        result = scheduleSet(active_f, network_utilization);
        if(result.size() < active_f.size()) {
            // fmt::print("only {}/{} active flows scheduled\n", active_solution.size(), active_f.size());
            return {};
        }
    }
    // schedule new flows (or all)
    auto req_solution = scheduleSet(req_f, network_utilization);
    result.insert(result.end(), req_solution.begin(), req_solution.end());

    return result;
}


auto HierarchicalHeuristicScheduling::scheduleSet(const robin_hood::unordered_set<common::FlowNodeID>& flow_set, common::NetworkUtilizationList& utilization) -> solver::solutionSet
{
    // create Flow heap
    // order by "total utilization" - package size/period
    std::vector<common::FlowNodeID> flow_list;
    flow_list.reserve(flow_set.size());
    for(const auto& key : flow_set) {
        flow_list.emplace_back(key);
    }

    const auto flow_comparison = get_flow_sorter(graph_, flow_sorter_type_);
    using PriorityQueue = std::priority_queue<common::FlowNodeID, std::vector<common::FlowNodeID>, decltype(std::ref(*flow_comparison))>;

    PriorityQueue flow_heap(std::ref(*flow_comparison), std::move(flow_list));
    solver::solutionSet result_set;


    while(not flow_heap.empty()) {
        auto current_flow_id = flow_heap.top();
        auto current_flow = graph_.getFlow(current_flow_id);
        flow_heap.pop();

        // calculate ratings for all configs of current Flow
        std::unique_ptr<AbstractConfigurationRating> rating_strategy = getRatingStrategy(
            config_rating_type_,
            graph_,
            utilization);
        rating_strategy->prepare();

        std::vector<std::pair<common::ConfigurationNodeID, float>> config_ratings;
        std::ranges::for_each(current_flow.configurations, [&](auto config_id) {
            auto rating = rating_strategy->rate(config_id);

            config_ratings.emplace_back(std::pair(config_id, rating));
        });

        // order the configs
        std::ranges::sort(config_ratings, [](auto lhs, auto rhs) {
            if(std::fabs(lhs.second - rhs.second) <= std::numeric_limits<float>::epsilon()) {
                return lhs.first < rhs.first;
            }
            return lhs.second < rhs.second;
        });

        for(const auto configID : config_ratings | std::views::keys) {
            // try placing configs until one works
            if(placeConfig(graph_.getConfiguration(configID), current_flow, utilization, placement_strategy_)) {
                result_set.emplace_back(std::pair(current_flow_id, configID));
                break;
            }
        }
    }


    // auto link_utilization = util::calculateLinkUtilization(graph_, network_utilization);
    // auto max = std::ranges::max_element(
    //     link_utilization, [&](auto lhs, auto rhs) {
    //         return lhs < rhs;
    //     }, [&](auto pair) {
    //         return pair.second;
    //     });
    // fmt::print("Max utilization on link: {}/{}\n", max->second, util::calculate_hyper_cycle(graph_));

    return result_set;
}
