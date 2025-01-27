#include "util/ScheduleVerifier.h"

#include <fmt/core.h>
#include <util/Constants.h>
#include <util/UtilFunctions.h>

struct validationError final : std::runtime_error
{
    explicit validationError(const std::string &msg)
        : std::runtime_error{msg} {}
};

auto findSourceNodeOfQueue(const MultiLayeredGraph &graph, const common::NetworkQueueID queue_id) -> common::NetworkNodeID
{
    /* The queue won't tell us it's source, only it's destination. The plan: Go through all nodes that the
     * destination node is connected to (which includes the node we're looking for) and go though all their
     * queue ids. When the queue id matches the one we started with, we know which node it belongs to. */
    const graph_structs::EgressQueue queue_struct = graph.getEgressQueue(queue_id);
    const common::NetworkNodeID queue_destination = queue_struct.destination;

    // Go through all nodes the destination node is connected to (suspects)
    for(const graph_structs::EgressQueue &reverse_queue : graph.getEgressQueuesOf(queue_destination)) {
        const common::NetworkNodeID suspect_node = reverse_queue.destination;

        // Go through all queues that are originating at the suspect.
        // If the queue id matches, we found the original source node!
        for(const graph_structs::EgressQueue &suspect_queue : graph.getEgressQueuesOf(suspect_node)) {
            if(suspect_queue.id == queue_id) {
                return suspect_node;
            }
        }
    }
    throw validationError("Network graph is not full duplex");
}

/**
 * Checks the following rules:
 * - All free egress slots must be at least 1 tick long
 * - All free egress slots must be correctly sorted from earliest to latest & not overlap
 * - There may be no free egress slots right next to each other, they have to be merged into one slot instead
 * - No free egress slot may exceed past the hyper cycle border
 */
void checkFreeSlotOrder(const MultiLayeredGraph &graph, const std::size_t hyper_cycle_len, const std::vector<common::FreeSlots> &free_slots)
{
    size_t port_index = 0;
    for(const auto &free_port_slots : free_slots) {
        const common::NetworkNodeID current_port_source = findSourceNodeOfQueue(graph, common::NetworkQueueID(port_index));
        const common::NetworkNodeID current_port_destination = graph.getEgressQueue(common::NetworkQueueID(port_index)).destination;
        size_t next_allowed_tick = 0;
        size_t slot_index = 0;
        for(const auto &free_single_slot : free_port_slots) {
            if(free_single_slot.start_time > free_single_slot.last_free_macro_tick) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"All free egress slots must be at least 1 tick long\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), free slot with index {} starts at tick {} and has {} as its last tick\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    free_single_slot.start_time,
                    free_single_slot.last_free_macro_tick);
                throw validationError("Free egress slot less than 1 tick long");
            }
            if(free_single_slot.start_time < next_allowed_tick) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"All free egress slots must be correctly sorted from earliest to latest & not overlap\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), free slot with index {} starts at tick {} even though the previous slot had {} as its last tick.\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    free_single_slot.start_time,
                    next_allowed_tick - 1);
                throw validationError("Free egress slots incorrectly sorted");
            }
            if(free_single_slot.start_time == next_allowed_tick && next_allowed_tick != 0) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"There may be no free egress slots right next to each other, they have to be merged into one slot instead\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), free slot with index {} starts at tick {} even though the previous slot had {} as its last tick.\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    free_single_slot.start_time,
                    next_allowed_tick - 1);
                throw validationError("Free egress slots incorrectly sorted");
            }
            if(free_single_slot.last_free_macro_tick >= hyper_cycle_len) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"No free egress slot may exceed past the hyper cycle border\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), free slot with index {} ends at tick {} even though the hyper cycle only lasts up to tick {}\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    free_single_slot.last_free_macro_tick,
                    hyper_cycle_len - 1);
                throw validationError("Free egress slots exceeded hyper cycle border");
            }
            next_allowed_tick = free_single_slot.last_free_macro_tick + 1;
            slot_index++;
        }
        port_index++;
    }
}

/**
 * Checks the following rules:
 * - All reserved egress slots must be at least 1 tick long
 * - All reserved egress slots must be correctly sorted from earliest to latest & not overlap
 * - No reserved egress slot may exceed past the hyper cycle border
 */
void checkReservedSlotOrder(const MultiLayeredGraph &graph, const std::size_t hyper_cycle_len, const std::vector<common::ReservedSlots> &reserved_slots)
{
    size_t port_index = 0;
    for(const auto &reserved_port_slots : reserved_slots) {
        const common::NetworkNodeID current_port_source = findSourceNodeOfQueue(graph, common::NetworkQueueID(port_index));
        const common::NetworkNodeID current_port_destination = graph.getEgressQueue(common::NetworkQueueID(port_index)).destination;
        size_t next_allowed_tick = 0;
        size_t slot_index = 0;
        for(const auto &reserved_single_slot : reserved_port_slots) {
            if(reserved_single_slot.start_time >= reserved_single_slot.next_slot_start) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"All reserved egress slots must be at least 1 tick long\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), reserved slot with index {} starts at tick {} and has {} as its last tick\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    reserved_single_slot.start_time,
                    reserved_single_slot.next_slot_start - 1);
                throw validationError("Reserved egress slot less than 1 tick long");
            }
            if(reserved_single_slot.start_time < next_allowed_tick) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"All reserved egress slots must be correctly sorted from earliest to latest & not overlap\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), reserved slot with index {} starts at tick {} even though the previous slot had {} as its last tick.\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    reserved_single_slot.start_time,
                    next_allowed_tick - 1);
                throw validationError("Reserved egress slots incorrectly sorted");
            }
            if(reserved_single_slot.next_slot_start > hyper_cycle_len) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"No reserved egress slot may exceed past the hyper cycle border\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), reserved slot with index {} ends at tick {} even though the hyper cycle only lasts up to tick {}\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_index,
                    reserved_single_slot.next_slot_start - 1,
                    hyper_cycle_len - 1);
                throw validationError("Reserved egress slots exceeded hyper cycle border");
            }
            next_allowed_tick = reserved_single_slot.next_slot_start;
            slot_index++;
        }
        port_index++;
    }
}
void checkSlotsForInverses(const MultiLayeredGraph &graph, const std::size_t hyper_cycle_len, const std::vector<common::FreeSlots> &free_slots, const std::vector<common::ReservedSlots> &reserved_slots)
{
    if(free_slots.size() != reserved_slots.size()) {
        throw validationError("The list for free slots and the list for reserved slots don't agree on the number of egress queues");
    }
    for(size_t port_index = 0; port_index < free_slots.size(); ++port_index) {
        const common::NetworkNodeID current_port_source = findSourceNodeOfQueue(graph, common::NetworkQueueID(port_index));
        const common::NetworkNodeID current_port_destination = graph.getEgressQueue(common::NetworkQueueID(port_index)).destination;
        common::FreeSlots free_port_slots = free_slots[port_index];
        common::ReservedSlots reserved_port_slots = reserved_slots[port_index];
        size_t next_tick = 0;
        size_t free_slots_cursor = 0;
        size_t reserved_slots_cursor = 0;
        while(next_tick != hyper_cycle_len) {
            if(free_slots_cursor < free_port_slots.size() && free_port_slots[free_slots_cursor].start_time == next_tick) {
                next_tick = free_port_slots[free_slots_cursor].last_free_macro_tick + 1;
                free_slots_cursor++;
            } else if(reserved_slots_cursor < reserved_port_slots.size() && reserved_port_slots[reserved_slots_cursor].start_time == next_tick) {
                next_tick = reserved_port_slots[reserved_slots_cursor].next_slot_start;
                reserved_slots_cursor++;
            } else {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"The free egress slots must be the inverse of the reserved egress slots\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), the last slot ended at tick {} and no free or reserved slot touches it\n",
                    port_index,
                    current_port_source.get(),
                    current_port_destination.get(),
                    next_tick);
                throw validationError("Free and reserved slots not inverses of each other");
            }
        }
    }
}
enum class FlowTrackerState {
    STRAND_OPEN,
    STRAND_CLOSED,
    FLOW_CLOSED,
};
struct StrandTracker
{
    std::size_t first_strand_tick;
    std::size_t last_strand_tick;
    std::size_t previous_frame_last_tick; // Is 0 if there was no last frame in the strand!
    std::vector<common::NetworkNodeID> path_taken;
};
struct FlowTracker
{
    common::FlowNodeID flow_id;
    // std::size_t package_size; // in byte
    // common::NetworkNodeID source;
    // common::NetworkNodeID destination;
    FlowTrackerState state;
    std::vector<StrandTracker> strands;
    std::size_t current_strand;
};

/// Return a fresh flow tracker with no processed slots for the given flow.
auto create_flow_tracker(const std::size_t hyper_cycle_len, const graph_structs::Flow &current_flow) -> FlowTracker
{
    auto new_flow_tracker = FlowTracker();
    new_flow_tracker.flow_id = current_flow.id;
    new_flow_tracker.state = FlowTrackerState::STRAND_CLOSED;
    if(current_flow.period < 1) {
        throw validationError("Flow period is less than 1");
    }
    if(hyper_cycle_len % current_flow.period != 0) {
        throw validationError("Hyper cycle is not multiple of flow period");
    }
    const size_t number_of_strands = hyper_cycle_len / current_flow.period;
    new_flow_tracker.strands = std::vector(number_of_strands, StrandTracker());
    for(size_t strand_index = 0; strand_index < number_of_strands; ++strand_index) {
        StrandTracker &current_strand = new_flow_tracker.strands[strand_index];
        current_strand.first_strand_tick = strand_index * current_flow.period;
        current_strand.last_strand_tick = ((strand_index + 1) * current_flow.period) - 1;
        current_strand.path_taken = std::vector<common::NetworkNodeID>();
        current_strand.previous_frame_last_tick = 0;
    }
    new_flow_tracker.current_strand = 0;
    return new_flow_tracker;
}
/**
 * Checks whether all scheduled flows don't break any rules:
 *
 * - Once a flow fulfilled its task fully, it must not have any other frames in the schedule
 * - Each frame must be exactly as long as its message size requires
 * - The frames of a strand must all fit inside the strand window
 * - Each strand of a flow must start at the flow's designated source node
 * - When a node receives a frame, it's impossible for another node to forward it before this node has forwarded it
 * - A node can't start forwarding a frame earlier than receive time + propagation & processing delay
 * - The path that a strand takes through a network may not cross itself
 * - The last frame of a strand must arrive at its destination before the strand window ends
 * - When the hyper cycle ends, all present flows must have accomplished their ambitions
 */
void check_flow_integrity(const MultiLayeredGraph &graph, const std::size_t hyper_cycle_len, const std::vector<common::ReservedSlots> &reserved_slots)
{
    std::unordered_map<common::FlowNodeID, FlowTracker> flow_trackers;
    std::size_t non_exhausted_ports = reserved_slots.size();
    auto slot_cursors = std::vector<size_t>(reserved_slots.size(), 0);
    auto next_slot_starts = std::vector<size_t>(reserved_slots.size(), 0);
    for(size_t port_index = 0; port_index < reserved_slots.size(); port_index++) {
        if(reserved_slots[port_index].empty()) {
            next_slot_starts[port_index] = hyper_cycle_len;
            non_exhausted_ports--;
        } else {
            next_slot_starts[port_index] = reserved_slots[port_index][0].start_time;
        }
    }
    while(non_exhausted_ports > 0) {
        // Find the port with the earliest next slot
        size_t minimum_value = hyper_cycle_len;
        size_t minimum_value_index = 0;
        for(size_t port_index = 0; port_index < reserved_slots.size(); port_index++) {
            if(next_slot_starts[port_index] < minimum_value) {
                minimum_value = next_slot_starts[port_index];
                minimum_value_index = port_index;
            }
        }
        if(minimum_value == hyper_cycle_len) {
            // I.e. all ports are fully exhausted
            // The while loop should terminate before this happens, this is an additional safeguard
            break;
        }
        const size_t current_port = minimum_value_index;

        const common::SingleReservedSlot current_slot = reserved_slots[current_port][slot_cursors[current_port]];
        const common::FlowNodeID current_flow_id = current_slot.flow_id;
        const graph_structs::Flow current_flow = graph.getFlow(current_flow_id);
        const common::NetworkNodeID current_port_source = findSourceNodeOfQueue(graph, common::NetworkQueueID(current_port));
        const common::NetworkNodeID current_port_destination = graph.getEgressQueue(common::NetworkQueueID(current_port)).destination;
        if(!flow_trackers.contains(current_flow_id)) {
            // Create new flow tracker
            FlowTracker current_flow_tracker = create_flow_tracker(hyper_cycle_len, current_flow);
            flow_trackers.try_emplace(current_flow_id, current_flow_tracker);
        }
        FlowTracker &current_flow_tracker = flow_trackers.at(current_flow_id);

        // Process flow
        if(current_flow_tracker.state == FlowTrackerState::FLOW_CLOSED) {
            fmt::print(stderr,
                "The schedule verifier found an inconsistency in the schedule.\n"
                "This rule has been violated:\n"
                "\"Once a flow fulfilled it's task fully, it must not have any other frames in the schedule\"\n"
                "Reason:\n"
                "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}, even though that flow has already been fully fulfilled\n",
                current_port,
                current_port_source.get(),
                current_port_destination.get(),
                slot_cursors[current_port],
                current_flow_id.get());
            throw validationError("Stray frame of a flow that is already closed");
        }
        StrandTracker &current_strand_tracker = current_flow_tracker.strands[current_flow_tracker.current_strand];

        // Check slot size
        const size_t actual_transmission_time = current_slot.next_slot_start - current_slot.start_time;
        const size_t required_transmission_time = util::calculate_transmission_delay(current_flow.frame_size);
        if(actual_transmission_time != required_transmission_time) {
            fmt::print(stderr,
                "The schedule verifier found an inconsistency in the schedule.\n"
                "This rule has been violated:\n"
                "\"Each frame must be exactly as long as its message size requires\"\n"
                "Reason:\n"
                "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}, which should be {} ticks long, but is instead {} ticks long\n",
                current_port,
                current_port_source.get(),
                current_port_destination.get(),
                slot_cursors[current_port],
                current_flow_id.get(),
                required_transmission_time,
                actual_transmission_time);
            throw validationError("Frame size does not match requirement");
        }

        // Check correct strand fit
        if(current_strand_tracker.first_strand_tick > current_slot.start_time || current_strand_tracker.last_strand_tick + 1 < current_slot.next_slot_start) {
            fmt::print(stderr,
                "The schedule verifier found an inconsistency in the schedule.\n"
                "This rule has been violated:\n"
                "\"The frames of a strand must all fit inside the strand window\"\n"
                "Reason:\n"
                "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}, which starts at tick {} and ends at tick {}, "
                "even though the strand that's supposed to contain it starts at tick {} and ends at tick {}.\n",
                current_port,
                current_port_source.get(),
                current_port_destination.get(),
                slot_cursors[current_port],
                current_flow_id.get(),
                current_slot.start_time,
                current_slot.next_slot_start - 1,
                current_strand_tracker.first_strand_tick,
                current_strand_tracker.last_strand_tick);
            throw validationError("Slot out of bounds of its containing strand window");
        }

        if(current_flow_tracker.state == FlowTrackerState::STRAND_CLOSED) {
            // Check correct source node
            if(current_flow.source != current_port_source) {
                fmt::print(stderr,
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"Each strand of a flow must start at the flow's designated source node\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), there is a start-of-strand slot with index {} belonging to flow {}, which should have instead been at node {}\n",
                    current_port,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_cursors[current_port],
                    current_flow_id.get(),
                    current_flow.source.get());
                throw validationError("Flow strand started at wrong source node");
            }
            // Add source node to path
            current_strand_tracker.path_taken.emplace_back(current_flow.source);
            current_flow_tracker.state = FlowTrackerState::STRAND_OPEN;
        } else { // This can only be (current_flow_tracker.state == STRAND_OPEN)
            // Check correct node handover
            const common::NetworkNodeID last_node_in_path = current_strand_tracker.path_taken[current_strand_tracker.path_taken.size() - 1];
            if(last_node_in_path != current_port_source) {
                fmt::print(stderr,
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"When a node receives a frame, it's impossible for another node to forward it before this node has forwarded it\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}, even though node {} should have this frame buffered right now\n",
                    current_port,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_cursors[current_port],
                    current_flow_id.get(),
                    last_node_in_path.get());
                throw validationError("A frame got forwarded by a node different from the node that last received the frame");
            }
            const std::size_t earliest_frame_availability = current_strand_tracker.previous_frame_last_tick + constant::propagation_delay + constant::processing_delay + 1;
            if(earliest_frame_availability > current_slot.start_time) {
                fmt::print(stderr,
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"A node can't start forwarding a frame earlier than receive time + propagation & processing delay\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}. "
                    "The slot gets sent at tick {}, but it only becomes available at tick {}\n",
                    current_port,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_cursors[current_port],
                    current_flow_id.get(),
                    current_slot.start_time,
                    earliest_frame_availability);
                throw validationError("A frame got forwarded before it got fully received");
            }
        }

        // Check whether the path crosses itself
        for(common::NetworkNodeID visited_node : current_strand_tracker.path_taken) {
            if(visited_node == current_port_destination) {
                fmt::print(stderr,
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"The path that a strand takes through a network may not cross itself\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}. "
                    "This strand already went through node {}.\n",
                    current_port,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_cursors[current_port],
                    current_flow_id.get(),
                    current_port_destination.get());
                throw validationError("Flow strand crossed itself in the topology");
            }
        }

        // Add next node in path to the path vector
        current_strand_tracker.path_taken.emplace_back(current_port_destination);
        // Register the end-of-frame time
        current_strand_tracker.previous_frame_last_tick = current_slot.next_slot_start - 1;

        // Check whether this is the destination
        if(current_port_destination == current_flow.destination) {
            // Deadline check
            const std::size_t true_arrival_time = current_slot.next_slot_start + constant::propagation_delay - 1;
            if(current_strand_tracker.last_strand_tick < true_arrival_time) {
                fmt::print(
                    "The schedule verifier found an inconsistency in the schedule.\n"
                    "This rule has been violated:\n"
                    "\"The last frame of a strand must arrive at its destination before the strand window ends\"\n"
                    "Reason:\n"
                    "In egress port {} ({} -> {}), there is a slot with index {} belonging to flow {}. "
                    "The slot arrives at tick {}, but it's deadline is at tick {}\n",
                    current_port,
                    current_port_source.get(),
                    current_port_destination.get(),
                    slot_cursors[current_port],
                    current_flow_id.get(),
                    true_arrival_time,
                    current_strand_tracker.last_strand_tick);
                std::cout << std::endl;
                throw validationError("Flow strand started at wrong source node");
            }
            // Close the strand / entire flow
            if(current_flow_tracker.current_strand + 1 >= current_flow_tracker.strands.size()) {
                // The flow has fulfilled its purpose
                current_flow_tracker.state = FlowTrackerState::FLOW_CLOSED;
            } else {
                // There is another strand
                ++current_flow_tracker.current_strand;
                current_flow_tracker.state = FlowTrackerState::STRAND_CLOSED;
            }
        }

        // Advance the cursor to the next slot
        ++slot_cursors[current_port];
        if(reserved_slots[current_port].size() <= slot_cursors[current_port]) {
            next_slot_starts[current_port] = hyper_cycle_len;
            --non_exhausted_ports;
        } else {
            next_slot_starts[current_port] = reserved_slots[current_port][slot_cursors[current_port]].start_time;
        }
    }

    // Now that all slots have been processed, all flows must be fully closed (complete)
    for(const auto &flow_tracker : flow_trackers | std::views::values) {
        if(flow_tracker.state != FlowTrackerState::FLOW_CLOSED) {
            fmt::print(
                "The schedule verifier found an inconsistency in the schedule.\n"
                "This rule has been violated:\n"
                "\"When the hyper cycle ends, all present flows must have accomplished their ambitions\"\n"
                "Reason:\n"
                "Flow {} is not completed",
                flow_tracker.flow_id.get());
            throw validationError("Flow present but unsuccessful");
        }
    }
}
auto verifySchedule(const common::NetworkUtilizationList &currently_active_utilization, const MultiLayeredGraph &graph, const std::size_t hyper_cycle) -> void
{
    const std::vector<common::FreeSlots> free_slots = currently_active_utilization.getFreeEgressSlots();
    const std::vector<common::ReservedSlots> reserved_slots = currently_active_utilization.getReservedEgressSlots();

    // Check whether free egress slots have positive length, are correctly sorted, are not overlapping, are properly merged, and don't exceed past hyper cycle border
    checkFreeSlotOrder(graph, hyper_cycle, free_slots);
    // Check whether reserved egress slots have positive length, are correctly sorted, are not overlapping, and don't exceed past hyper cycle border
    checkReservedSlotOrder(graph, hyper_cycle, reserved_slots);
    // Check whether reserved & free egress slots are each other's inverses
    checkSlotsForInverses(graph, hyper_cycle, free_slots, reserved_slots);

    // Check whether all flows are correctly placed
    check_flow_integrity(graph, hyper_cycle, reserved_slots);
}
