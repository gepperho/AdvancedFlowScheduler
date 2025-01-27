#include "solver/UtilizationList.h"
#include "util/Constants.h"
#include "util/UtilFunctions.h"
#include <algorithm>
#include <fmt/core.h>
#include <fmt/ranges.h>

#include <fstream>
#include <queue>
#include <ranges>

common::NetworkUtilizationList::NetworkUtilizationList(const std::size_t number_of_network_links,
                                                       const std::size_t hyper_cycle,
                                                       const std::size_t sub_cycle)
    : hyper_cycle_(hyper_cycle),
      sub_cycle_(sub_cycle),
      number_of_network_links_(number_of_network_links)
{
    clear();
}


auto common::NetworkUtilizationList::getFreeEgressSlots() const -> const std::vector<FreeSlots>&
{
    return free_egress_slots_;
}

auto common::NetworkUtilizationList::getReservedEgressSlots() const -> const std::vector<ReservedSlots>&
{
    return reserved_egress_slots_;
}

auto common::NetworkUtilizationList::searchTransmissionOpportunities(const graph_structs::Configuration& current_config,
                                                                     const graph_structs::Flow& current_flow,
                                                                     const std::size_t first_release_time,
                                                                     const std::size_t deadline) const noexcept
    -> std::vector<SlotReservationRequest>
{
    // Tuple: (Network Queue ID, start sending time, end sending time, arrival time)
    auto required_slots = std::vector<SlotReservationRequest>();
    required_slots.reserve(current_config.path.size());
    auto running_arrival_time = first_release_time;

    const auto current_transmission_delay = util::calculate_transmission_delay(current_flow.frame_size);
    const auto effective_deadline = deadline - current_transmission_delay - constant::propagation_delay;

    const auto constraint_arrival_to_slot_end_large_enough = [current_transmission_delay, &running_arrival_time](const auto& slot) {
        // slot has not ended before the package could be sent
        // +1 so that we have the actual slot end
        return slot.last_free_macro_tick + 1 < running_arrival_time + current_transmission_delay;
    };

    const auto constraint_slot_start_before_deadline = [effective_deadline](const auto& slot) {
        // there is enough time in the slot to transmit the frame and for the frame to reach the next hop before the deadline
        // +1 so we have the actual slot end
        return slot.start_time <= effective_deadline;
    };

    const auto constraint_sufficient_slot_time_before_deadline = [current_transmission_delay, effective_deadline, &running_arrival_time](const auto& slot) {
        // there is enough time in the slot to transmit the frame and for the frame to reach the next hop before the deadline
        // +1 so we have the actual slot end
        return std::max(slot.start_time, running_arrival_time) + current_transmission_delay <= std::min(slot.last_free_macro_tick + 1, effective_deadline + current_transmission_delay);
    };

    // Note: we use all_of to be able to break early
    const bool is_schedulable = std::ranges::all_of(current_config.path, [&](auto network_link_id) {
        auto filtered = free_egress_slots_[network_link_id.get()]
            | std::ranges::views::drop_while(constraint_arrival_to_slot_end_large_enough)
            | std::ranges::views::take_while(constraint_slot_start_before_deadline)
            | std::ranges::views::filter(constraint_sufficient_slot_time_before_deadline)
            | std::ranges::views::take(1);

        const auto it = filtered.begin();
        if(it == filtered.end()) {
            return false;
        }
        const auto free_slot = *it;

        auto earliest_send_time = std::max(free_slot.start_time, running_arrival_time);

        required_slots.emplace_back(network_link_id, earliest_send_time, earliest_send_time + current_transmission_delay, running_arrival_time);
        // adding the transmission delay here makes it store-and-forward switching. Replace it by some realistic constant for cut-through.
        running_arrival_time = earliest_send_time + current_transmission_delay + constant::propagation_delay + constant::processing_delay;

        // for the frame a slot was found in each network segment
        return true;
    });

    if(not is_schedulable) {
        return {};
    }
    return required_slots;
}


auto common::NetworkUtilizationList::removeConfigs(const std::vector<FlowNodeID>& flows) -> void
{
    for(auto i = std::size_t{0}; i < reserved_egress_slots_.size(); ++i) {

        // remove reserved slots
        std::vector<SingleReservedSlot> to_be_removed;
        auto [remove_start, remove_end] = std::ranges::stable_partition(reserved_egress_slots_.at(i),
                                                                        [&](const auto& reserved_slot) {
                                                                            // inverse check, so the elements to keep are in the beginning
                                                                            return not util::vector_contains(flows, reserved_slot.flow_id);
                                                                        });
        std::ranges::copy(remove_start, remove_end, std::back_inserter(to_be_removed));
        reserved_egress_slots_.at(i).erase(remove_start, reserved_egress_slots_.at(i).end());

        std::ranges::sort(to_be_removed, [](auto lhs, auto rhs) {
            return lhs.start_time < rhs.start_time;
        });
        // add the time to free slots
        std::ranges::for_each(to_be_removed, [&](auto triple) {
            freeSlot(i, triple);
        });

        // remove arrivals accordingly
        auto [arrival_remove_start, arrival_remove_end] = std::ranges::stable_partition(std::begin(flow_arrival_.at(i)),
                                                                                        std::end(flow_arrival_.at(i)),
                                                                                        [&](const auto& pair) {
                                                                                            // inverse check, so the elements to keep are in the beginning
                                                                                            return not util::vector_contains(flows, pair.first);
                                                                                        });

        flow_arrival_.at(i).erase(arrival_remove_start, arrival_remove_end);
    }
}

auto common::NetworkUtilizationList::freeSlot(const std::size_t network_segment_id, const SingleReservedSlot& slot) -> void
{
    const auto& [begin, end, flow_id, config_id] = slot;

    auto slot_it = free_egress_slots_[network_segment_id].begin();
    auto next_slot_it = std::next(slot_it, 1);

    // handle corner cases
    // slot is before the first slot but not adjacent
    if(slot_it->start_time > end) {
        free_egress_slots_.at(network_segment_id).insert(slot_it, SingleFreeSlot{begin, end - 1});
        return;
    }
    // slot is before and the first observed slot and adjacent
    if(slot_it->start_time == end) {
        slot_it->start_time = begin;
        return;
    }

    // slot is after the last element but not adjacent
    const auto last_element_it = std::prev(free_egress_slots_.at(network_segment_id).end(), 1);
    if(last_element_it->last_free_macro_tick < begin - 1) {
        free_egress_slots_[network_segment_id].insert(free_egress_slots_.at(network_segment_id).end(), SingleFreeSlot{begin, end - 1});
        return;
    }
    // slot is after the last free slot and adjacent
    if(last_element_it->last_free_macro_tick == begin - 1) {
        last_element_it->last_free_macro_tick = end - 1;
    }

    // regular cases
    while(next_slot_it != free_egress_slots_.at(network_segment_id).end()) {
        auto& current_slot = *slot_it;
        auto& next_slot = *next_slot_it;

        // the slot to be freed is after the currently observed slots
        if(next_slot.last_free_macro_tick < begin - 1) {
            std::advance(slot_it, 1);
            std::advance(next_slot_it, 1);
            continue;
        }

        // the slot to be freed exactly gaps the observed slots
        if(current_slot.last_free_macro_tick == begin - 1 and end == next_slot.start_time) {
            // combine the two slots
            current_slot.last_free_macro_tick = next_slot.last_free_macro_tick;
            free_egress_slots_.at(network_segment_id).erase(next_slot_it);
            return;
        }
        // the slot to be freed is adjacent to only the first observed slot
        if(current_slot.last_free_macro_tick == begin - 1) {
            current_slot.last_free_macro_tick = end - 1;
            return;
        }
        // the slot to be freed is adjacent to only the second observed slot
        if(next_slot.start_time == end) {
            next_slot.start_time = begin;
            return;
        }
        // the to be freed slot is between the observed slots without being exactly adjacent to any
        if(current_slot.last_free_macro_tick < begin - 1 and next_slot.start_time > end) {
            // insert new slot
            free_egress_slots_.at(network_segment_id).insert(next_slot_it, SingleFreeSlot{begin, end - 1});
            return;
        }
        std::advance(slot_it, 1);
        std::advance(next_slot_it, 1);
    }
}

auto common::NetworkUtilizationList::printFreeTimeSlots(const std::string& name) const -> void
{
    std::ofstream utilization_file;
    utilization_file.open("utilization_" + name + ".txt");
    utilization_file << "[\n";
    std::string output;
    for(const auto& egress_port_list : free_egress_slots_) {
        auto slot_occupation = std::vector(hyper_cycle_, 1);
        for(const auto& free_slots : egress_port_list) {
            auto [start, end] = free_slots;
            for(auto i = start; i <= end; ++i) {
                slot_occupation[i] = 0;
            }
        }
        auto occupation_string = format("[{}],\n", fmt::join(slot_occupation, ", "));
        output += occupation_string;
    }
    // remove the last
    output.pop_back();
    output.pop_back();
    utilization_file << output;
    utilization_file << "\n]" << std::endl;
    utilization_file.close();
}
auto common::NetworkUtilizationList::reserveSlot(const SlotReservationRequest& requiredSlot,
                                                 const FlowNodeID flow_id,
                                                 const ConfigurationNodeID config_id)
    -> bool
{
    addFlowArrival(requiredSlot.egress_queue, flow_id, requiredSlot.arrival_time);
    // store the slot in the "global" required list
    reserved_egress_slots_.at(requiredSlot.egress_queue.get()).emplace_back(requiredSlot.start_time, requiredSlot.next_slot_start, flow_id, config_id);

    // update the free slots
    const auto free_slot_iter = std::ranges::find_if(
        free_egress_slots_.at(requiredSlot.egress_queue.get()),
        [&](auto slot_pair) {
            return slot_pair.start_time <= requiredSlot.start_time and slot_pair.last_free_macro_tick >= requiredSlot.next_slot_start - 1;
        });
    if(free_slot_iter == free_egress_slots_.at(requiredSlot.egress_queue.get()).end()) {
        return false;
    }
    auto& free_slot = *free_slot_iter;
    const auto slot_start_time = free_slot.start_time;

    if(slot_start_time == requiredSlot.start_time and free_slot.last_free_macro_tick == requiredSlot.next_slot_start - 1) {
        // old slot is completely used and can be removed
        free_egress_slots_.at(requiredSlot.egress_queue.get()).erase(free_slot_iter);
    } else if(slot_start_time == requiredSlot.start_time) {
        // start of old slot can be updated, no partitioning
        free_slot.start_time = requiredSlot.next_slot_start;
    } else if(free_slot.last_free_macro_tick + 1 == requiredSlot.next_slot_start) {
        // end of old slot can be updated, no partitioning
        free_slot.last_free_macro_tick = requiredSlot.start_time - 1;
    } else {
        // partitioning of the old slot
        const auto new_slot = SingleFreeSlot{free_slot.start_time, requiredSlot.start_time - 1};
        free_slot.start_time = requiredSlot.next_slot_start;
        free_egress_slots_.at(requiredSlot.egress_queue.get()).insert(free_slot_iter, new_slot);
    }
    return true;
}

auto common::NetworkUtilizationList::getFlowArrivalsCopy(NetworkQueueID egress_queue_id) const -> std::vector<std::pair<FlowNodeID, std::size_t>>
{
    return flow_arrival_.at(egress_queue_id.get());
}

auto common::NetworkUtilizationList::addFlowArrival(NetworkQueueID egress_queue_id, FlowNodeID flow_id, std::size_t arrival_time) -> void
{
    flow_arrival_[egress_queue_id.get()].emplace_back(std::make_pair(flow_id, arrival_time));
}


auto common::NetworkUtilizationList::clear() -> void
{
    reserved_egress_slots_.clear();
    flow_arrival_.clear();
    free_egress_slots_.clear();

    free_egress_slots_.resize(number_of_network_links_);
    std::ranges::for_each(free_egress_slots_, [hyper_cycle = hyper_cycle_](auto& link_slots) {
        link_slots.emplace_back(SingleFreeSlot{std::size_t{0}, hyper_cycle - 1});
    });

    reserved_egress_slots_.resize(number_of_network_links_);
    flow_arrival_.resize(number_of_network_links_);
}

auto common::NetworkUtilizationList::sortReservedEgressSlots() -> void
{
    for(auto& egress_queue : reserved_egress_slots_) {
        std::ranges::sort(egress_queue);
    }
}
auto common::NetworkUtilizationList::compute_frames_per_hc(const std::size_t period) const -> std::size_t
{
    return hyper_cycle_ / period;
}
auto common::NetworkUtilizationList::getSubCycle() const -> size_t
{
    return sub_cycle_;
}
