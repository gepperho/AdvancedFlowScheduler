#include "solver/scheduler/CelfFlowQueuing.h"
#include "solver/ConfigurationRating/ConfigurationRatingFactory.h"
#include "solver/FlowSorting/FlowSorterFactory.h"
#include <queue>

CelfFlowQueuing::CelfFlowQueuing(MultiLayeredGraph& graph,
                                 const celf_rating::CelfRatingTypes rating_type,
                                 const placement::ConfigPlacementTypes placement_type)
    : graph_(graph),
      config_rating_type_(rating_type),
      placement_strategy_(placement_type)
{
}

auto CelfFlowQueuing::solve(const MultiLayeredGraph& graph,
                            const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                            const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                            common::NetworkUtilizationList& network_utilization) -> solver::solutionSet
{

    solver::solutionSet result;

    if(not active_f.empty()) {
        // offensive planning is conducted
        result = scheduleSet(active_f, network_utilization);
        if(result.size() < active_f.size()) {
            // fmt::print("only {}/{} active flows scheduled\n", active_solution.size(), active_f.size());
            return {};
        }
    }

    // defensive planning/reqF of offensive planning is conducted
    auto req_solution = scheduleSet(req_f, network_utilization);
    result.insert(result.end(), req_solution.begin(), req_solution.end());

    return result;
}
auto CelfFlowQueuing::scheduleSet(const robin_hood::unordered_set<common::FlowNodeID>& set, common::NetworkUtilizationList& network_utilization) -> solver::solutionSet
{
    robin_hood::unordered_set<common::FlowNodeID> covered_flows;

    std::vector<common::ConfigurationNodeID> heap_vector;
    solver::solutionSet solution_set;

    std::ranges::for_each(set, [&](auto flow_id) {
        auto flow = graph_.getFlow(flow_id);
        heap_vector.insert(heap_vector.begin(), flow.configurations.begin(), flow.configurations.end());
    });

    const auto rating_function = getRatingStrategy(config_rating_type_, graph_, network_utilization);
    std::ranges::for_each(graph_.getConfigurations(), [&](const auto tuple) {
        auto rating = rating_function->rate(tuple.first);
        config_rating_[tuple.first] = rating;
    });


    auto config_comparison = [&config_rating = config_rating_](const common::ConfigurationNodeID lhs_config_id, const common::ConfigurationNodeID rhs_config_id) {
        const auto& lhs_rating = config_rating.at(lhs_config_id);
        const auto& rhs_rating = config_rating.at(rhs_config_id);

        return lhs_rating < rhs_rating;
    };

    using PriorityQueue = std::priority_queue<common::ConfigurationNodeID, std::vector<common::ConfigurationNodeID>, decltype(config_comparison)>;
    PriorityQueue heap(config_comparison, heap_vector);

    while(not heap.empty()) {
        auto top_config = heap.top();
        auto& current_config = graph_.getConfiguration(top_config);
        heap.pop();

        if(covered_flows.contains(current_config.flow)) {
            // skip if flow is already covered
            config_rating_[top_config] = std::pair(-1, -1);
            continue;
        }

        if(updated_.contains(top_config)) {
            // pick, since it is an updated value
            if(pick_config(current_config, network_utilization)) {
                covered_flows.insert(current_config.flow);
                solution_set.emplace_back(std::pair(current_config.flow, top_config));
                rating_function->pick(top_config);
                updated_.clear();
            }
            continue;
        }

        // reevaluate
        rating_function->prepare();
        config_rating_[top_config] = rating_function->rate(top_config);

        // take or reinsert
        if(config_rating_[top_config] >= config_rating_[heap.top()]) {
            // better than the next best option
            if(pick_config(current_config, network_utilization)) {
                covered_flows.insert(current_config.flow);
                solution_set.emplace_back(std::pair(current_config.flow, top_config));
                rating_function->pick(top_config);
                updated_.clear();
            }
        } else {
            // became worse after the update
            heap.push(top_config);
            updated_.insert(top_config);
        }
    }

    return solution_set;
}

auto CelfFlowQueuing::pick_config(const graph_structs::Configuration& config_id, common::NetworkUtilizationList& network_utilization) -> bool
{
    const auto success = placeConfig(config_id, graph_.getFlow(config_id.flow), network_utilization, placement_strategy_);
    if(success) {
        updated_.clear();
    }
    config_rating_[config_id.id] = std::pair(-1, -1);
    return success;
}
