#pragma once

#include "UtilizationList.h"
#include "graph/GraphStructs.h"

namespace placement {

enum class ConfigPlacementTypes {
    ASAP = 0,
    BALANCED = 1,
    HERMES = 2,
};

[[nodiscard]] inline auto to_int(ConfigPlacementTypes type) -> int
{
    return static_cast<int>(type);
}

[[nodiscard]] auto placeConfig(const graph_structs::Configuration& currentConfig,
                               const graph_structs::Flow& currentFlow,
                               common::NetworkUtilizationList& utilizationList,
                               ConfigPlacementTypes placementType)
    -> bool;

/**
 * Tries to place the given config. E.g. checks if the remaining free egress slots suffice for the whole hyper-cycle.
 * If yes, the required slots are reserved and the free slots updated accordingly.
 * Note: the method expects the deadline to be the same as the period.
 * @param current_config
 * @param current_flow
 * @param utilizationList
 * @return returns false if the config can not be scheduled without violating the deadline.
 */
[[nodiscard]] auto placeConfigASAP(const graph_structs::Configuration& current_config,
                                   const graph_structs::Flow& current_flow,
                                   common::NetworkUtilizationList& utilizationList)
    -> bool;


[[nodiscard]] auto placeConfigBalanced(const graph_structs::Configuration& current_config,
                                       const graph_structs::Flow& current_flow,
                                       common::NetworkUtilizationList& utilizationList)
    -> bool;


/**
 * Reservation method for the Hermes algorithm
 * @param configuration
 * @param flow
 * @param utilizationList
 * @param egress_queue
 * @param latest_offset
 * @return
 */
[[nodiscard]] auto hermesPlacement(const graph_structs::Configuration& configuration,
                                   const graph_structs::Flow& flow,
                                   common::NetworkUtilizationList& utilizationList,
                                   common::NetworkQueueID egress_queue,
                                   std::size_t latest_offset)
    -> bool;
} // namespace placement