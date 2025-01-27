#include "solver/Placement.h"
#include "util/UtilFunctions.h"



auto placement::placeConfig(const graph_structs::Configuration& currentConfig,
                            const graph_structs::Flow& currentFlow,
                            common::NetworkUtilizationList& utilizationList,
                            const ConfigPlacementTypes placementType)
    -> bool
{
    switch(placementType) {
        using enum ConfigPlacementTypes;
    case ASAP:
        return placeConfigASAP(currentConfig, currentFlow, utilizationList);
    case BALANCED:
        return placeConfigBalanced(currentConfig, currentFlow, utilizationList);
    case HERMES:
        // the HERMES placement requires additional information and thus needs to be called directly
        return false;
    }
    return false;
}

auto placement::placeConfigASAP(const graph_structs::Configuration& current_config,
                                const graph_structs::Flow& current_flow,
                                common::NetworkUtilizationList& utilizationList)
    -> bool
{
    const auto frames_per_hyper_cycle = utilizationList.compute_frames_per_hc(current_flow.period);
    // Tuple: (Network Queue ID, start sending time, end sending time, arrival time)
    std::vector<std::vector<common::SlotReservationRequest>> required_slots;
    required_slots.reserve(frames_per_hyper_cycle);

    const auto success = std::ranges::all_of(std::views::iota(std::size_t{0}, frames_per_hyper_cycle), [&](const std::size_t frame_index) {
        const auto reserve_slots = utilizationList.searchTransmissionOpportunities(current_config, current_flow, frame_index * current_flow.period, (frame_index + 1) * current_flow.period);
        if(reserve_slots.empty()) {
            return false;
        }
        required_slots.push_back(reserve_slots);
        return true;
    });
    if(not success) {
        return false;
    }

    // reserve the required slots
    std::ranges::for_each(required_slots, [&](auto& required_slots_of_period) {
        std::ranges::for_each(required_slots_of_period, [&](auto& required_slot) {
            utilizationList.reserveSlot(required_slot, current_flow.id, current_config.id);
        });
    });

    return true;
}




auto placement::placeConfigBalanced(const graph_structs::Configuration& current_config,
                                    const graph_structs::Flow& current_flow,
                                    common::NetworkUtilizationList& utilizationList)
    -> bool
{
    /*
     * check period (divide period by min_period)
     * for every min_period offset until period:
     * -- check end to end time
     * -- -- works similar to placeConfigASAP
     * use the offset with the best end to end time
     */

    const auto number_of_sub_cycles = current_flow.period / utilizationList.getSubCycle();
    std::vector<std::vector<std::vector<common::SlotReservationRequest>>> reserved_slots;
    reserved_slots.resize(number_of_sub_cycles);
    const auto frames_per_hyper_cycle = utilizationList.compute_frames_per_hc(current_flow.period);

    for(std::size_t sub_cycle_index = 0; sub_cycle_index < number_of_sub_cycles; ++sub_cycle_index) {
        std::vector<std::vector<common::SlotReservationRequest>> reserved_slots_for_offset;

        if(std::ranges::all_of(std::views::iota(std::size_t{0}, frames_per_hyper_cycle), [&](const std::size_t frame_index) {
               const auto current_release_time = sub_cycle_index * utilizationList.getSubCycle() + frame_index * current_flow.period;
               const auto deadline = (frame_index + 1) * current_flow.period;
               const auto reserved_slot = utilizationList.searchTransmissionOpportunities(current_config, current_flow, current_release_time, deadline);

               if(not reserved_slot.empty()) {
                   reserved_slots_for_offset.push_back(reserved_slot);
               }
               return not reserved_slot.empty();
           })) {
            reserved_slots[sub_cycle_index] = std::move(reserved_slots_for_offset);
        }
    }

    auto valid_options = std::ranges::filter_view(reserved_slots, [](auto& hyper_cycle_reservation) {
        return not hyper_cycle_reservation.empty();
    });
    if(valid_options.empty()) {
        // no valid placement found
        return false;
    }

    const auto best_option = *std::ranges::min_element(valid_options,
                                                       std::less(),
                                                       [&](const std::vector<std::vector<common::SlotReservationRequest>>& offset1_reservations) {
                                                           auto get_frame_e2e_time = [](const auto& reservations) {
                                                               return reservations.back().next_slot_start - reservations.front().arrival_time;
                                                           };
                                                           const auto& slowest_frame_1 = std::ranges::max_element(offset1_reservations, [&get_frame_e2e_time](const auto& reservations1, const auto& reservations2) {
                                                               const auto r1_e2e = get_frame_e2e_time(reservations1);
                                                               const auto r2_e2e = get_frame_e2e_time(reservations2);
                                                               return r1_e2e <= r2_e2e;
                                                           });
                                                           const auto frame_1_e2e = get_frame_e2e_time(*slowest_frame_1);
                                                           return frame_1_e2e;
                                                       });


    // reserve the required slots
    std::ranges::for_each(best_option, [&](const auto& required_slots_of_period) {
        std::ranges::for_each(required_slots_of_period, [&](const auto& required_slot) {
            utilizationList.reserveSlot(required_slot, current_flow.id, current_config.id);
        });
    });

    return true;
}
auto placement::hermesPlacement(const graph_structs::Configuration& configuration,
                                const graph_structs::Flow& flow,
                                common::NetworkUtilizationList& utilizationList,
                                const common::NetworkQueueID egress_queue,
                                const std::size_t latest_offset)
    -> bool
{
    auto transmission_delay = util::calculate_transmission_delay(flow.frame_size);
    auto filtered_view = std::ranges::filter_view(utilizationList.getFreeEgressSlots().at(egress_queue.get()),
                                                  [latest_offset, transmission_delay](auto slot) {
                                                      auto start = slot.start_time;
                                                      auto end = slot.last_free_macro_tick;
                                                      // -1 since the end slot is included in the free slots
                                                      return start <= latest_offset and end - start >= transmission_delay;
                                                  });
    const auto latest_slot = std::ranges::max_element(filtered_view,
                                                      [](auto lhs, auto rhs) {
                                                          // find latest slot
                                                          return lhs.start_time < rhs.start_time;
                                                      });

    const auto start_time = std::min(latest_slot->last_free_macro_tick - transmission_delay + 1, latest_offset);
    const auto next_slot_start_time = start_time + transmission_delay;
    // Tuple: (Network Queue ID, start sending time, end sending time, arrival time) - arrival time is incorrect since unknown at this time
    return utilizationList.reserveSlot(
        common::SlotReservationRequest{.egress_queue = egress_queue, .start_time = start_time, .next_slot_start = next_slot_start_time, .arrival_time = start_time},
        flow.id, configuration.id);
}
