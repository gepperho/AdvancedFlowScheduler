#pragma once
#include <solver/UtilizationList.h>

/**
 * Check the reserved egress slots and free egress slots for inconsistencies.
 */
auto verifySchedule(const common::NetworkUtilizationList &currently_active_utilization, const MultiLayeredGraph &graph, std::size_t hyper_cycle)
    -> void;
