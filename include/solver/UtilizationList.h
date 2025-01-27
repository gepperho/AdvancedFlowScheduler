#pragma once

#include <Typedefs.h>
#include <graph/MultiLayeredGraph.h>
#include <ranges>
#include <vector>

namespace common {

struct SingleFreeSlot
{
    std::size_t start_time;
    std::size_t last_free_macro_tick;
};

struct SingleReservedSlot
{
    std::size_t start_time;
    std::size_t next_slot_start;
    FlowNodeID flow_id;
    ConfigurationNodeID config_id;

    bool operator==(const SingleReservedSlot& s) const = default;

    auto operator<=>(const SingleReservedSlot& s) const
    {
        if(start_time != s.start_time) {
            return start_time <=> s.start_time;
        }
        return next_slot_start <=> s.next_slot_start;
    };
};

struct SlotReservationRequest
{
    NetworkQueueID egress_queue;
    std::size_t start_time;
    std::size_t next_slot_start;
    std::size_t arrival_time;
};

using FreeSlots = std::vector<SingleFreeSlot>;
using ReservedSlots = std::vector<SingleReservedSlot>;


class NetworkUtilizationList
{

public:
    NetworkUtilizationList() = default;

    ~NetworkUtilizationList() = default;

    NetworkUtilizationList(std::size_t number_of_network_links, std::size_t hyper_cycle, std::size_t sub_cycle);

    [[nodiscard]] auto getFreeEgressSlots() const -> const std::vector<FreeSlots>&;

    [[nodiscard]] auto getReservedEgressSlots() const -> const std::vector<ReservedSlots>&;

    auto sortReservedEgressSlots() -> void;

    /**
     * clears all internal lists, sets capacities and enters initial values if needed (e.g. free slots)
     */
    auto clear() -> void;



    /**
     * searches for slots with configs that belong to a given flow and "frees" the slots
     * @param flows
     */
    auto removeConfigs(const std::vector<FlowNodeID>& flows) -> void;

    [[nodiscard]] auto getReservedSlotsOf(NetworkQueueID egress_queue, FlowNodeID flowNodeId) -> auto
    {
        auto x = std::views::all(reserved_egress_slots_[egress_queue.get()])
            | std::views::filter([flowNodeId](auto& reserved_slot) {
                     return reserved_slot.flow_id.get() == flowNodeId.get();
                 });
        return x;
    }


    [[maybe_unused]] auto printFreeTimeSlots(const std::string& name) const -> void;

    [[nodiscard]] auto getFlowArrivalsCopy(NetworkQueueID egress_queue_id) const -> std::vector<std::pair<FlowNodeID, std::size_t>>;

    auto addFlowArrival(NetworkQueueID egress_queue_id, FlowNodeID flow_id, std::size_t arrival_time) -> void;

    /**
     * use with caution. placeConfigASAP should be the way to go for most situations
     * @param requiredSlot
     * @param flow_id
     * @param config_id
     */
    auto reserveSlot(const SlotReservationRequest& requiredSlot,
                     FlowNodeID flow_id,
                     ConfigurationNodeID config_id) -> bool;


    [[nodiscard]] auto compute_frames_per_hc(std::size_t period) const -> std::size_t;

    auto getSubCycle() const -> std::size_t;

    auto searchTransmissionOpportunities(const graph_structs::Configuration& current_config,
                                         const graph_structs::Flow& current_flow,
                                         std::size_t first_release_time,
                                         std::size_t deadline) const noexcept
        -> std::vector<SlotReservationRequest>;

private:
    /**
     * inserts the given time range as a free slot
     * @param network_segment_id
     * @param slot
     */
    auto freeSlot(std::size_t network_segment_id, const SingleReservedSlot& slot) -> void;

    /*
     * stores time slots where the egress port is free as tuples (x,y)
     * The time span x to y (including y) are free
     *
     * The free egress slots are sorted.
     */
    std::vector<FreeSlots> free_egress_slots_;
    /*
     * stores the time slots that are reserved as a triple (x,y,z).
     * The time span x to y-1 (thus excluding y) is reserved for config z.
     *
     * The reserved slots are not sorted.
     */
    std::vector<ReservedSlots> reserved_egress_slots_;
    std::size_t hyper_cycle_;
    std::size_t sub_cycle_;
    std::size_t number_of_network_links_;
    std::vector<std::vector<std::pair<FlowNodeID, std::size_t>>> flow_arrival_;
};

} // namespace common
