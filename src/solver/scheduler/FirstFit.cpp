
#include "solver/scheduler/FirstFit.h"
#include "solver/Placement.h"

FirstFit::FirstFit(MultiLayeredGraph& graph)
    : graph_(graph)
{
}

auto FirstFit::name() -> std::string
{
    return "FirstFit";
}

auto FirstFit::solve(const MultiLayeredGraph& graph,
                     const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                     const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                     common::NetworkUtilizationList& network_utilization)
    -> solver::solutionSet
{

    solver::solutionSet result_set;
    // add all active_f flows to the network utilization
    add_flows(active_f, network_utilization, result_set);

    if(result_set.size() < active_f.size()) {
        // one missing -> return empty set
        return solver::solutionSet();
    }

    // add all req_f flows to the network utilization
    add_flows(req_f, network_utilization, result_set);

    return result_set;
}

auto FirstFit::add_flows(const robin_hood::unordered_set<common::FlowNodeID>& search_f, common::NetworkUtilizationList& utilization, solver::solutionSet& result_set) -> void
{
    // make sure we add the flows ordered
    std::vector<common::FlowNodeID> flow_list;
    flow_list.reserve(search_f.size());
    std::ranges::for_each(search_f, [&](auto f) { flow_list.emplace_back(f); });
    std::ranges::sort(flow_list);

    for(auto flow_id : flow_list) {
        // add all search_f flows to the network utilization
        auto& flow = graph_.getFlow(flow_id);
        auto config_id = flow.configurations.front();
        const auto& config = graph_.getConfiguration(config_id);

        if(placement::placeConfigASAP(config, flow, utilization)) {
            result_set.emplace_back(std::make_pair(flow_id, config_id));
        }
    }
}
