#include "solver/scheduler/EarliestDeadlineFirst.h"
#include "fmt/core.h"
#include "util/Timer.h"
#include "util/UtilFunctions.h"
#include <queue>

EarliestDeadlineFirst::EarliestDeadlineFirst(MultiLayeredGraph& graph)
    : graph_(graph),
      hyper_cycle_(util::calculate_hyper_cycle(graph)) // needs to be updated whenever the flow set changes, i.e., when calling solve
{
}

auto EarliestDeadlineFirst::solve(const MultiLayeredGraph& graph, const robin_hood::unordered_set<common::FlowNodeID>& active_f,
                                  const robin_hood::unordered_set<common::FlowNodeID>& req_f, common::NetworkUtilizationList& network_utilization)
    -> solver::solutionSet
{
    if(skip_run_) {
        // prevent every second run, starting with the first. -> only offensive planning is conducted
        skip_run_ = false;
        return {};
    }
    skip_run_ = true;

    hyper_cycle_ = util::calculate_hyper_cycle(graph);

    solver::solutionSet input;
    input.reserve(graph_.getNumberOfFlows());

    std::vector approximated_traffic(graph_.getNumberOfEgressQueues(), 0.0f);
    constexpr float factor = 1.1f;

    std::ranges::for_each(active_f, [&](auto flow_id) {
        auto& flow = graph_.getFlow(flow_id);
        input.emplace_back(std::pair(flow_id, flow.configurations.front()));
        auto& config = graph_.getConfiguration(flow.configurations.front());
        auto traffic = static_cast<float>(util::calculate_transmission_delay(flow.frame_size) * (hyper_cycle_ / flow.period)) * factor;
        for(auto egress_queue_id : config.path) {
            approximated_traffic[egress_queue_id.get()] += traffic;
        }
    });

    std::vector<common::FlowNodeID> req_f_vector;
    req_f_vector.reserve(req_f.size());
    std::ranges::for_each(req_f, [&](auto flow_id) {
        req_f_vector.emplace_back(flow_id);
    });
    std::ranges::sort(req_f_vector, [](auto lhs, auto rhs) {
        return lhs.get() < rhs.get();
    });

    std::vector<std::pair<common::FlowNodeID, common::ConfigurationNodeID>> missing;
    std::ranges::for_each(req_f_vector, [&](auto flow_id) {
        auto& flow = graph_.getFlow(flow_id);
        auto& config = graph_.getConfiguration(flow.configurations.front());
        auto traffic = static_cast<float>(util::calculate_transmission_delay(flow.frame_size) * (hyper_cycle_ / flow.period)) * factor;
        auto it = config.path.begin();
        while(it != config.path.end()) {
            auto egress_queue_id = *it;
            if(approximated_traffic[egress_queue_id.get()] + traffic < static_cast<float>(hyper_cycle_)) {
                approximated_traffic[egress_queue_id.get()] += traffic;

            } else {
                // revert changes and go to next flow
                while(it != config.path.begin()) {
                    it = std::prev(it);
                    egress_queue_id = *it;
                    approximated_traffic[egress_queue_id.get()] -= traffic;
                }

                missing.emplace_back(std::pair(flow_id, config.id));
                return;
            }
            it = std::next(it);
        }
        input.emplace_back(std::pair(flow_id, flow.configurations.front()));
    });

    auto timer = Timer();
    auto temp = simulateEdfPlacement(input, network_utilization);
    if(temp.has_value()) {
        // start from 0 if the traffic approximation was to optimistic
        input.insert(input.end(), missing.begin(), missing.end());
        missing = std::move(input);
    }

    for(auto next_flow_config_pair : missing) {
        input.emplace_back(next_flow_config_pair);

        timer.reset();
        temp = simulateEdfPlacement(input, network_utilization);
        if(temp.has_value()) {
            input.pop_back();
        }
    }

    return input;
}

auto EarliestDeadlineFirst::simulateEdfPlacement(const solver::solutionSet& configs, common::NetworkUtilizationList& utilizationList) const
    -> std::optional<common::FlowNodeID>
{
    utilizationList.clear();
    // create an "inbox" for every egress port
    // tuple: ConfigurationId, time, package_size, period
    auto inbox = std::vector<std::vector<std::tuple<common::ConfigurationNodeID, std::size_t, std::size_t, std::size_t, std::size_t>>>(graph_.getNumberOfEgressQueues());

    // create a time list for every egress slot -> priority queue with (egressPortId, time) pairs
    std::vector<std::pair<common::NetworkQueueID, std::size_t>> time_vector;
    time_vector.reserve(graph_.getNumberOfEgressQueues());
    for(auto id = std::size_t{0}; id < graph_.getNumberOfEgressQueues(); ++id) {
        time_vector.emplace_back(std::pair(common::NetworkQueueID{id}, 0));
    }

    auto time_comparison = [](const std::pair<common::NetworkQueueID, std::size_t>& lhs, const std::pair<common::NetworkQueueID, std::size_t>& rhs) {
        if(lhs.second == rhs.second) {
            return lhs.first > rhs.first; // tie-breaker with IDs
        }
        return lhs.second > rhs.second; // smaller time at the front
    };
    using PriorityQueue = std::priority_queue<std::pair<common::NetworkQueueID, std::size_t>, std::vector<std::pair<common::NetworkQueueID, std::size_t>>, decltype(time_comparison)>;
    PriorityQueue time_list(time_comparison, time_vector);

    // assign the flows to the egress ports' inboxes
    for(auto [flow_id, config_id] : configs) {
        auto& current_config = graph_.getConfiguration(config_id);
        auto& egress_port = current_config.path.front();
        auto& current_flow = graph_.getFlow(flow_id);

        // emplace a frame for every period of the flow
        for(auto i = std::size_t{0}; i < hyper_cycle_ / current_flow.period; ++i) {
            inbox[egress_port.get()].emplace_back(config_id, i * current_flow.period, current_flow.frame_size, current_flow.period, (i + 1) * current_flow.period);
        }
    }

    while(not time_list.empty()) {
        auto [current_egress_port, current_time] = time_list.top();
        time_list.pop();
        auto& current_inbox = inbox[current_egress_port.get()];

        if(not current_inbox.empty()) {

            // find the frame with the earliest deadline (to be scheduled next)
            auto [frame_config_id, arrival_time, package_size, period, deadline] =
                std::reduce(current_inbox.begin(), current_inbox.end(),
                            std::tuple(common::ConfigurationNodeID{0}, hyper_cycle_ * 10, std::size_t{5000}, hyper_cycle_ * 10, hyper_cycle_ * 10),
                            [current_time](auto& lhs, auto& rhs) {
                                auto [lhs_id, lhs_time, lhs_package_size, lhs_period, lhs_deadline] = lhs;
                                auto [rhs_id, rhs_time, rhs_package_size, rhs_period, rhs_deadline] = rhs;

                                if(lhs_time <= current_time and rhs_time <= current_time) {
                                    if(lhs_deadline != rhs_deadline) {
                                        return lhs_deadline < rhs_deadline ? lhs : rhs;
                                    }
                                    if(lhs_period != rhs_period) {
                                        return lhs_period < rhs_period ? lhs : rhs;
                                    }
                                    return lhs_id < rhs_id ? lhs : rhs;
                                }
                                return lhs_time < rhs_time ? lhs : rhs;
                            });

            if(arrival_time <= current_time) {
                //  remove the frame from the inbox
                std::erase_if(current_inbox, [frame_config_id, arrival_time](const auto& element) {
                    const bool id_check = std::get<0>(element).get() == frame_config_id.get();
                    const bool arrival_time_check = std::get<1>(element) == arrival_time;
                    return id_check and arrival_time_check;
                });

                // schedule the frame
                auto& frame_config = graph_.getConfiguration(frame_config_id);
                const auto transmission_time = util::calculate_transmission_delay(package_size);
                // find next hop
                auto next_hop_iterator = frame_config.path.begin();
                while(next_hop_iterator->get() != current_egress_port.get()) {
                    next_hop_iterator = std::next(next_hop_iterator);
                }
                auto next_hop = std::next(next_hop_iterator);

                if(next_hop != frame_config.path.end()) {
                    auto next_hop_id = *next_hop;
                    // check if deadline can be satisfied (with an additional switching delay, since it is not the last hop)
                    auto time_ready_at_next_hop = current_time + transmission_time + constant::propagation_delay + constant::processing_delay;
                    if(time_ready_at_next_hop >= deadline) {
                        // deadline is missed -> complete failure.
                        return frame_config.flow;
                    }

                    // add the frame (with updated timing information) to the next hop
                    inbox[next_hop_id.get()].emplace_back(frame_config_id, time_ready_at_next_hop, package_size, period, deadline);
                } else {
                    // check if deadline can be satisfied (withOUT switching delay, since it is the last hop)
                    if(current_time + transmission_time + constant::propagation_delay > deadline) {
                        // a deadline is missed -> complete failure.
                        return frame_config.flow;
                    }
                }
                // update utilization list
                utilizationList.reserveSlot(common::SlotReservationRequest{.egress_queue = current_egress_port, .start_time = current_time, .next_slot_start = current_time + transmission_time, .arrival_time = arrival_time},
                                            frame_config.flow,
                                            frame_config.id);
                // egress port is ready again at:
                time_list.push(std::pair(current_egress_port, current_time + transmission_time));
            } else {
                // the inbox has packages arriving later -> check again later
                time_list.push(std::pair(current_egress_port, current_time + 1));
            }
        } else {
            if(current_time < hyper_cycle_) {
                // there are currently no packages in the inbox -> check again later
                time_list.push(std::pair(current_egress_port, current_time + 1));
            }
        }
    }
    // if all assignments work: success, return true
    return std::nullopt;
}
