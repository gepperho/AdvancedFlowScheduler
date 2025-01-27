#pragma once

#include "Constants.h"
#include "solver/UtilizationList.h"
#include "solver/scheduler/AbstractScheduler.h"
#include <algorithm>
#include <scenario/TimeStep.h>
#include <vector>

namespace util {

template<class T>
auto vector_contains(const std::vector<T>& vec, const T& val) -> bool
{
    return std::ranges::find(vec, val) != vec.end();
}


inline auto calculate_hyper_cycle(const MultiLayeredGraph& graph) -> std::size_t
{
    return std::transform_reduce(
        graph.getFlows().cbegin(),
        graph.getFlows().cend(),
        std::size_t{1},
        [](auto v1, auto v2) { return std::lcm(v1, v2); },
        [](auto key_value) { return key_value.second.period; });
}

inline auto calculate_hyper_cycle(const std::vector<io::TimeStep>& scenario) -> std::size_t
{
    robin_hood::unordered_set<std::size_t> periods;
    std::ranges::for_each(scenario, [&](const auto& time_step) {
        std::ranges::for_each(time_step.add_flows, [&](const auto& flow) {
            periods.insert(flow.period);
        });
    });

    return std::reduce(periods.cbegin(), periods.cend(),
                       std::size_t{1},
                       [](auto v1, auto v2) {
                           return std::lcm(v1, v2);
                       });
}

inline auto calculate_gcd_period(const std::vector<io::TimeStep>& scenario) -> std::size_t
{
    robin_hood::unordered_set<std::size_t> unique_periods;
    std::ranges::for_each(scenario, [&](const auto& time_step) {
        std::ranges::for_each(time_step.add_flows, [&](const auto& flow) {
            unique_periods.insert(flow.period);
        });
    });
    auto gcd = std::reduce(unique_periods.cbegin(), unique_periods.cend(),
                           *unique_periods.begin(),
                           [](auto v1, auto v2) {
                               return std::gcd(v1, v2);
                           });
    return gcd;
}

/**
 * Calculates the transmission delay in micro seconds.
 * @param package_size
 * @return
 */
inline auto calculate_transmission_delay(const std::size_t package_size) -> std::size_t
{
    return (package_size * 8) / constant::network_speed;
}


/**
 * calculate the link usage of the given graph with the given utilization pattern.
 * The utilization is given in reserved macro ticks per egress queue.
 * @param graph
 * @param utilization
 * @return
 */
inline auto calculateLinkUtilization(const MultiLayeredGraph& graph, const common::NetworkUtilizationList& utilization) -> std::unordered_map<common::NetworkQueueID, std::size_t>
{
    std::unordered_map<common::NetworkQueueID, std::size_t> link_utilization;

    for(const auto& egress_queue : graph.getEgressQueues()) {

        const auto current_id = egress_queue.id;
        const auto start = utilization.getReservedEgressSlots()[current_id.get()].begin();
        const auto end = utilization.getReservedEgressSlots()[current_id.get()].end();

        const auto reserved = std::transform_reduce(start, end,
                                                    std::size_t{0},
                                                    std::plus<>(),
                                                    [&](const auto& tuple) {
                                                        const auto& [inner_start, inner_end, flow_id, config_id] = tuple;
                                                        return inner_end - inner_start;
                                                    });
        // cast from size_t to float ... maybe use double
        // or just transform_reduce directly into float or double
        link_utilization[current_id] = reserved;
    }

    return link_utilization;
}

inline auto calculateAverageLinkUtilization(const std::unordered_map<common::NetworkQueueID, std::size_t>& link_utilization) -> float
{
    return std::transform_reduce(link_utilization.begin(),
                                 link_utilization.end(),
                                 0.f,
                                 std::plus<>(),
                                 [&link_utilization](const auto& pair) {
                                     return static_cast<float>(pair.second) / static_cast<float>(link_utilization.size());
                                 });
}


template<class Container, class Extractor>
auto calculate_ingress_traffic_impl(const MultiLayeredGraph& graph,
                                    const Container& container,
                                    Extractor&& extractor)
    -> double
{
    auto total_traffic = std::transform_reduce(std::cbegin(container),
                                               std::cend(container),
                                               0.,
                                               std::plus<>(),
                                               [&](const auto& tuple) {
                                                   const auto id = std::invoke(std::forward<Extractor>(extractor), tuple);
                                                   const auto& flow = graph.getFlow(id);
                                                   // package[bit] * (periods per ms)
                                                   return static_cast<double>(flow.frame_size) * 8 * (1000. / static_cast<double>(flow.period));
                                               });
    // bit/ms -> k bit/s
    return total_traffic / 1000; // make it in M bit/s
}

/**
 * calculates the ingress traffic of the given flows in M bit/s
 * @param scheduled_flows
 * @param graph
 * @return
 */
inline auto calculate_ingress_traffic(const robin_hood::unordered_set<common::FlowNodeID>& scheduled_flows,
                                      const MultiLayeredGraph& graph) -> double
{
    return calculate_ingress_traffic_impl(graph,
                                          scheduled_flows,
                                          std::identity{});
}

/**
 * calculates the ingress traffic of the given flows in M bit/s
 * @param scheduled_flows
 * @param graph
 * @return
 */
inline auto calculate_ingress_traffic(const solver::solutionSet& scheduled_flows,
                                      const MultiLayeredGraph& graph) -> double
{
    return calculate_ingress_traffic_impl(graph, scheduled_flows,
                                          [](const auto& tuple) { return tuple.first; });
}

template<class Container, class Extractor>
inline auto calculate_number_of_frames_impl(const MultiLayeredGraph& graph,
                                            const Container& container,
                                            Extractor&& extractor) -> std::size_t
{
    auto number_of_frames = std::transform_reduce(std::cbegin(container),
                                                  std::cend(container),
                                                  std::size_t{0},
                                                  std::plus<>(),
                                                  [&, hyper_cycle = calculate_hyper_cycle(graph)](const auto& tuple) {
                                                      const auto id = std::invoke(std::forward<Extractor>(extractor), tuple);
                                                      const auto& flow = graph.getFlow(id);
                                                      return hyper_cycle / flow.period;
                                                  });
    return number_of_frames;
}


inline auto calculate_number_of_frames(const solver::solutionSet& scheduled_flows,
                                       const MultiLayeredGraph& graph) -> std::size_t
{
    return calculate_number_of_frames_impl(graph, scheduled_flows,
                                           [](const auto& tuple) { return tuple.first; });
}

inline auto calculate_number_of_frames(const robin_hood::unordered_set<common::FlowNodeID>& scheduled_flows,
                                       const MultiLayeredGraph& graph) -> std::size_t
{
    return calculate_number_of_frames_impl(graph, scheduled_flows, std::identity{});
}

inline auto to_upper(std::string s) -> std::string
{
    std::ranges::transform(s, s.begin(), [](const unsigned char c) {
        return std::toupper(c);
    });
    return s;
}

/**
 * Reports the maximum amount of frames that are queued simultaneously at a single switch.
 * @param utilizationList
 * @param graph
 * @return
 */
inline auto calculate_max_queue_size(common::NetworkUtilizationList& utilizationList, const MultiLayeredGraph& graph) -> std::size_t
{
    std::size_t global_max_queue_size = 0;
    utilizationList.sortReservedEgressSlots();

    for(const graph_structs::EgressQueue& egress_queue : graph.getEgressQueues()) {
        if(egress_queue.end_device) {
            // skip end devices
            continue;
        }
        std::size_t max_queue_Size = 0;
        std::size_t sent_counter = 0;

        auto sorted_arrivals = utilizationList.getFlowArrivalsCopy(egress_queue.id);
        if(sorted_arrivals.size() < global_max_queue_size) {
            continue;
        }
        std::ranges::sort(sorted_arrivals, [&](auto lhs, auto rhs) {
            return lhs.second < rhs.second;
        });

        auto arrival_it = sorted_arrivals.begin();
        std::size_t received_so_far_count = 0;

        for(const auto& slot : utilizationList.getReservedEgressSlots()[egress_queue.id.get()]) {
            const auto& [slot_start, next_slot_start, slot_flow, slot_config] = slot;

            while(arrival_it != sorted_arrivals.end() and arrival_it->second <= slot_start) {
                ++received_so_far_count;
                arrival_it = std::next(arrival_it);
            }

            max_queue_Size = std::max(max_queue_Size, received_so_far_count - sent_counter);
            ++sent_counter;
        }

        global_max_queue_size = std::max(global_max_queue_size, max_queue_Size);
    }

    return global_max_queue_size;
}

/**
 * Creates a list with the number of frame forwarding entries a scheduling table would have for each egress port
 * @param utilizationList
 * @return vector of scheduling table sizes
 */
inline auto calculate_scheduling_table_sizes(const common::NetworkUtilizationList& utilizationList) -> std::vector<std::size_t>
{
    std::vector<std::size_t> forwarding_list_lengths;
    forwarding_list_lengths.reserve(utilizationList.getReservedEgressSlots().size());

    std::ranges::for_each(utilizationList.getReservedEgressSlots(), [&](auto& egress_port_forwarding_vector) {
        forwarding_list_lengths.emplace_back(egress_port_forwarding_vector.size());
    });
    return forwarding_list_lengths;
}

template<class T>
concept arithmetic = std::is_arithmetic_v<T>;

/**
 * Returns the average value of a vector of arithmetic type.
 * Uses float values for the division and assumes that the sum of the vector can be computed without any overflows.
 * @tparam T arithmetic type
 * @param vec list of numbers
 * @return average value
 */
template<class T>
    requires arithmetic<T>
auto get_average_value(const std::vector<T>& vec)
{
    auto sum = std::reduce(vec.cbegin(), vec.cend());
    return static_cast<float>(sum) / static_cast<float>(vec.size());
}

/**
 * to_lower function for strings
 * @param str
 * @return
 */
inline auto to_lower(const std::string& str) -> std::string
{
    std::string temp = str;
    std::ranges::transform(str, temp.begin(), ::tolower);
    return temp;
}


} // namespace util
