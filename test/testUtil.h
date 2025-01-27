#pragma once

#include <gtest/gtest.h>
#include "Typedefs.h"
#include "graph/MultiLayeredGraph.h"
#include "solver/UtilizationList.h"
#include <fmt/core.h>
#include <iostream>

inline auto create_route_string(const std::vector<common::NetworkQueueID>& route, MultiLayeredGraph& graph, common::NetworkNodeID source)
    -> std::string
{
    std::stringstream s;
    s << source.get();
    for(const auto hop : route) {
        auto next_dest = graph.getEgressQueue(hop).destination;
        s << fmt::format(" {}", next_dest.get());
    }
    return s.str();
}

inline auto print_route(const std::vector<common::NetworkQueueID>& route, MultiLayeredGraph& graph, const common::NetworkNodeID source)
{
    fmt::print("{}\n", create_route_string(route, graph, source));
}

inline auto print_egress_queue_to_link_mapping(const MultiLayeredGraph& graph) -> void
{
    for(std::size_t i = 0; i < graph.getNumberOfNetworkNodes(); ++i) {
        auto id = common::NetworkNodeID{i};
        for(const auto& egress_port : graph.getEgressQueuesOf(id)) {
            fmt::print("link {}-{}\tPort: {}\n", id.get(), egress_port.destination.get(), egress_port.id.get());
        }
    }
    std::cout << std::endl;
}

inline auto check_reservation_overlaps(const common::NetworkUtilizationList& utilization_list)
{
    for(const auto& queue : utilization_list.getReservedEgressSlots()) {
        if(queue.empty()) {
            continue;
        }
        for(std::size_t i = 0; i < queue.size() - 1; ++i) {
            auto& slot1 = queue.at(i);
            for(std::size_t j = i + 1; j < queue.size(); ++j) {
                auto& slot2 = queue.at(j);
                // slots should not overlap (end of the first and start of the second can be euqal
                EXPECT_NE(slot1.start_time, slot2.start_time);
                EXPECT_NE(slot1.next_slot_start, slot2.next_slot_start);

                if(slot1.start_time < slot2.start_time) {
                    EXPECT_LE(slot1.next_slot_start, slot2.start_time);
                } else {
                    EXPECT_LE(slot2.next_slot_start, slot1.start_time);
                }
            }
        }
    }
}