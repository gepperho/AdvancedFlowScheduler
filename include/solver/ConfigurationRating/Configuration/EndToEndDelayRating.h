#pragma once
#include "solver/ConfigurationRating/AbstractConfigurationRating.h"

class EndToEndDelayRating final : public AbstractConfigurationRating
{
public:
    EndToEndDelayRating(MultiLayeredGraph& graph, common::NetworkUtilizationList& utilization)
        : graph_(graph),
          utilization_(utilization)
    {}

    auto toString() -> std::string_view override
    {
        return "End-to-End-delay";
    }

    auto rate(const common::ConfigurationNodeID config_id)
        -> float override
    {
        const auto& current_config = graph_.getConfiguration(config_id);
        const auto& current_flow = graph_.getFlow(current_config.flow);
        const auto hyper_cycle = util::calculate_hyper_cycle(graph_);

        // TODO: revisit the code, since it is currently mostly a code duplicate

        const auto packages_per_hyper_cycle = hyper_cycle / current_flow.period;
        auto required_slots = std::vector<std::vector<std::tuple<common::NetworkQueueID, std::size_t, std::size_t>>>(packages_per_hyper_cycle);
        std::vector<std::size_t> arrival_times;

        arrival_times.reserve(packages_per_hyper_cycle);
        for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {
            arrival_times.emplace_back(i * current_flow.period);
        }

        const auto current_transmission_delay = util::calculate_transmission_delay(current_flow.frame_size);

        // Note: we use all_of to be able to break early
        const auto is_schedulable = std::ranges::all_of(current_config.path, [&](auto network_link_id) {
            for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {

                const bool last_hop = current_config.path.back().get() == network_link_id.get();

                auto earliest_arrival = arrival_times[i];
                auto next_slot = std::ranges::find_if(utilization_.getFreeEgressSlots()[network_link_id.get()], [&](auto& free_slot) {
                    auto earliest_send_time = std::max(free_slot.start_time, earliest_arrival);

                    // 1) slot has not ended before the package arrives
                    auto condition_1 = free_slot.last_free_macro_tick >= earliest_arrival;
                    // 2) transmission is done before the deadline is reached. There is no switching delay on the last hop (when arriving)
                    auto condition_2 = earliest_send_time + current_transmission_delay + constant::propagation_delay + (last_hop ? 0 : constant::processing_delay) <= (i + 1) * current_flow.period;
                    // 3) slot (with the earliest send time) is long enough for the package
                    //      -> the -1 is needed, since the "start time micro tick" is also used
                    auto condition_3 = free_slot.last_free_macro_tick >= earliest_send_time - 1 + current_transmission_delay;

                    return (condition_1 and condition_2 and condition_3);
                });

                if(next_slot == utilization_.getFreeEgressSlots()[network_link_id.get()].end()) {
                    // no slot available
                    return false;
                }
                // store the egress slot to be used
                auto free_slot = *next_slot;
                auto earliest_send_time = std::max(free_slot.start_time, earliest_arrival);

                required_slots[i].emplace_back(std::make_tuple(network_link_id, earliest_send_time, earliest_send_time + current_transmission_delay));
                arrival_times[i] = earliest_send_time + current_transmission_delay + constant::propagation_delay + constant::processing_delay;
            }
            // for all packages a slot was found for each network segment
            return true;
        });

        if(not is_schedulable) {
            return std::numeric_limits<float>::infinity();
        }


        // auto inevitable_delay = current_config.path.size() * (current_transmission_delay + constant::processing_delay) - constant::processing_delay;

        auto total_queueing_delay = std::size_t{0};

        // get queuing delays
        for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {
            const auto start_time = i * current_flow.period;
            total_queueing_delay += arrival_times[i] - start_time; // - inevitable_delay;
        }

        // return the average queuing delay
        return static_cast<float>(total_queueing_delay) / static_cast<float>(packages_per_hyper_cycle);
    }

private:
    MultiLayeredGraph& graph_;
    common::NetworkUtilizationList& utilization_;
};
