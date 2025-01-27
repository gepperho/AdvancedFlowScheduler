#include "solver/scheduler/Hermes.h"
#include "solver/Placement.h"
#include "solver/UtilizationList.h"
#include "util/UtilFunctions.h"
#include <ranges>

Hermes::Hermes(MultiLayeredGraph& graph)
    : graph_(graph)
{}

auto Hermes::name() -> std::string
{
    return "Hermes";
}

auto Hermes::solve(const MultiLayeredGraph& graph,
                   const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                   const robin_hood::unordered_set<common::FlowNodeID>& req_f,
                   common::NetworkUtilizationList& network_utilization)
    -> solver::solutionSet
{
    const auto phases = divPhases();
    if(not phases.has_value()) {
        return solver::solutionSet();
    }
    const auto frame_utilization = assignFrameUtilization();
    auto result = schedule(phases.value(), frame_utilization, network_utilization);

    return result;
}

auto Hermes::divPhases()
    -> std::optional<robin_hood::unordered_map<common::NetworkQueueID, std::size_t>>
{
    std::vector<std::size_t> phases(graph_.getNumberOfEgressQueues(), 0);
    for(auto& egress_queue : graph_.getEgressQueues()) {
        // handle unused links
        if(egress_queue.used_by.empty()) {
            phases[egress_queue.id.get()] = 1;
        }
    }

    std::size_t phi = 1;

    while(std::ranges::any_of(phases, [](auto v) { return v == 0; })) {


        // loop paths via flows
        for(const auto& [flow_id, flow] : graph_.getFlows()) {
            auto config_id = flow.configurations.front(); // HERMES assumes to have only one path per flow
            const auto& config = graph_.getConfiguration(config_id);

            auto next_path_segment = getNextPathSegmentForPhase(phases, config.path, phi);

            if(not next_path_segment.has_value()) {
                continue;
            }

            // check if the next_path_segment is somewhere else not the last segment
            auto delay = std::ranges::any_of(graph_.getConfigurations(), [&](auto tuple) {
                auto& [current_id, current_config] = tuple;

                if(not util::vector_contains(current_config.path, next_path_segment.value())) {
                    return false;
                }
                // next_path_segment is part of the current path
                auto current_next_segment = getNextPathSegmentForPhase(phases, current_config.path, phi);

                if(not current_next_segment.has_value()) {
                    return false;
                }

                // if they are not the same, the next_path_segment needs to be delayed
                return next_path_segment.value().get() != current_next_segment.value().get();
            });
            if(not delay) {
                phases[next_path_segment.value().get()] = phi;
            }
        }

        if(phi > 1000) {
            // emergency exit if a deadlock occurs
            return {};
        }

        ++phi;
    }

    robin_hood::unordered_map<common::NetworkQueueID, std::size_t> phase_map;
    phase_map.reserve(graph_.getNumberOfEgressQueues());

    common::NetworkQueueID i{0};
    std::ranges::for_each(phases, [&](auto& phase_number) {
        phase_map[i] = phase_number;
        ++i;
    });
    return phase_map;
}

auto Hermes::getNextPathSegmentForPhase(std::vector<std::size_t>& phases, const std::vector<common::NetworkQueueID>& path, const std::size_t phi)
    -> std::optional<common::NetworkQueueID>
{
    const auto iter = std::ranges::find(
        path.rbegin(), path.rend(),
        std::size_t{0},
        [&phases](auto id) {
            return phases.at(id.get());
        });

    if(iter == path.rend()) {
        return std::nullopt;
    }

    if(iter != path.rbegin()) {
        // ensure we don't assign phi to adjacent links
        auto prev_id = *std::prev(iter);
        if(phases.at(prev_id.get()) == phi) {
            return prev_id;
        }
    }

    return *iter;
}

auto Hermes::assignFrameUtilization() -> std::vector<float>
{
    std::vector<float> frame_utilization;
    frame_utilization.resize(graph_.getNumberOfFlows());
    for(const auto& [flow_id, flow] : graph_.getFlows()) {
        auto config_id = flow.configurations.front();
        const auto& config = graph_.getConfiguration(config_id);
        auto temp_utilization = static_cast<float>(util::calculate_transmission_delay(flow.frame_size))
            / static_cast<float>(flow.period) * static_cast<float>(config.path.size());
        frame_utilization[config_id.get()] = temp_utilization;
    }
    return frame_utilization;
}

auto Hermes::schedule(const robin_hood::unordered_map<common::NetworkQueueID, std::size_t>& phases,
                      const std::vector<float>& frame_utilization,
                      common::NetworkUtilizationList& network_utilization)
    -> solver::solutionSet
{
    solver::solutionSet solution;

    const auto hyper_cycle = util::calculate_hyper_cycle(graph_);
    auto max_phi = std::ranges::max_element(phases, std::less(), [&](auto tuple) { return tuple.second; })->second;
    for(std::size_t phase = 1; phase <= max_phi; ++phase) {
        //
        // loop phases
        for(const auto& link_id : phases
                | std::ranges::views::filter([phase](auto& tuple) { return tuple.second == phase; })
                | std::ranges::views::transform([](auto& tuple) { return tuple.first; })) {
            // loop links
            const auto& egress_queue = graph_.getEgressQueue(link_id);
            auto used_by_copy = egress_queue.used_by;
            std::ranges::sort(used_by_copy, [&](auto lhs_config, auto rhs_config) {
                // order by largest frame_utilization first
                return frame_utilization.at(lhs_config.get()) > frame_utilization.at(rhs_config.get());
            });
            for(const auto config_id : used_by_copy) {
                // loop configs/flows
                const auto& config = graph_.getConfiguration(config_id);
                const auto& flow = graph_.getFlow(config.flow);

                const auto frames_per_hc = std::size_t{hyper_cycle / flow.period};
                for(std::size_t frame_no = 0; frame_no < frames_per_hc; ++frame_no) {
                    auto deadline = flow.period * (frame_no + 1);
                    auto next_link = std::ranges::find(config.path, link_id);
                    next_link = std::next(next_link);
                    std::size_t prev_offset = [&] {
                        if(next_link == config.path.end()) {
                            // the current link is the last hop
                            return deadline;
                        }
                        // get the reserved slot on the next link of the current period (there should be only one)
                        auto reserved_slots = network_utilization.getReservedSlotsOf(*next_link, flow.id);
                        for(auto& slot : reserved_slots | std::views::filter([deadline, &flow](const auto& current_slot) {
                                             auto start = current_slot.start_time;
                                             return start < deadline and start >= deadline - flow.period; // within the current period
                                         })) {
                            return slot.start_time - constant::processing_delay;
                        }
                        // default if something went wrong
                        return deadline;
                    }();
                    // latest_offset: current frame must be sent latest at this time
                    std::size_t latest_offset = std::min(deadline, prev_offset) - constant::propagation_delay - util::calculate_transmission_delay(flow.frame_size);
                    if(not placement::hermesPlacement(config, flow, network_utilization, egress_queue.id, latest_offset)) {
                        // assignment failed
                        return solver::solutionSet();
                    }
                }
            }
        }
    }

    for(const auto& [config_id, config] : graph_.getConfigurations()) {
        solution.emplace_back(std::make_pair(config.flow, config_id));
    }

    return solution;
}
