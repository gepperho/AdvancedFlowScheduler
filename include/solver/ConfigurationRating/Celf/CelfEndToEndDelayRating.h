#pragma once
#include "solver/ConfigurationRating/AbstractCelfRating.h"
#include "util/UtilFunctions.h"

class CelfEndToEndDelayRating final : public AbstractCelfRating
{
public:
    CelfEndToEndDelayRating(MultiLayeredGraph& graph, common::NetworkUtilizationList& utilization)
        : graph_(graph),
          utilization_(utilization)
    {}

    auto toString() -> std::string_view override
    {
        return "Celf-End-to-End-delay";
    }

    auto rate(const common::ConfigurationNodeID config_id)
        -> std::pair<float, float> override
    {
        auto& current_config = graph_.getConfiguration(config_id);
        const auto& current_flow = graph_.getFlow(current_config.flow);
        const auto hyper_cycle = util::calculate_hyper_cycle(graph_);

        // TODO: revisit the code, since it is currently mostly a code duplicate

        const auto packages_per_hyper_cycle = hyper_cycle / current_flow.period;
        std::vector<std::size_t> arrival_times;
        std::vector<std::size_t> first_hop;

        arrival_times.reserve(packages_per_hyper_cycle);
        first_hop.reserve(packages_per_hyper_cycle);
        for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {
            arrival_times.emplace_back(i * current_flow.period);
        }

        auto current_transmission_delay = util::calculate_transmission_delay(current_flow.frame_size);

        // Note: we use all_of to be able to break early
        const auto is_schedulable = std::ranges::all_of(current_config.path, [&](auto network_link_id) {
            for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {

                const bool last_hop = current_config.path.back().get() == network_link_id.get();

                auto earliest_arrival = arrival_times[i];
                auto next_slot = std::ranges::find_if(utilization_.getFreeEgressSlots()[network_link_id.get()], [&](auto& free_slot) {
                    auto earliest_send_time = std::max(free_slot.last_free_macro_tick, earliest_arrival);

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

                if(first_hop.size() < i + 1) {
                    first_hop.emplace_back(earliest_send_time);
                }

                arrival_times[i] = earliest_send_time + current_transmission_delay + constant::propagation_delay + constant::processing_delay;
            }
            // for all packages a slot was found for each network segment
            return true;
        });

        if(not is_schedulable) {
            return std::pair(-1.f, -1.f);
        }

        // auto inevitable_delay = current_config.path.size() * (current_transmission_delay + constant::processing_delay) - constant::processing_delay;

        auto max_delay = -1 * static_cast<int>(current_flow.period);

        // get queuing delays
        for(std::size_t i = 0; i < packages_per_hyper_cycle; ++i) {
            const auto start_time = i * current_flow.period + first_hop[i];
            const auto delay = static_cast<int>(arrival_times[i] - start_time);
            max_delay = std::max(max_delay, delay);
        }

        // return the average queuing delay
        const auto period = large_constant / static_cast<float>(current_flow.period);
        const auto frame_size = static_cast<float>(current_flow.frame_size);
        const auto slack = static_cast<float>(current_flow.period - max_delay) / 1000;
        return std::pair(period + frame_size + slack, current_config.id.get());
    }

private:
    const float large_constant = 10000000;

    MultiLayeredGraph& graph_;
    common::NetworkUtilizationList& utilization_;
};
