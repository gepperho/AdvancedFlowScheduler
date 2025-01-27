#pragma once

#include "AbstractScheduler.h"
/**
* Simplified implementation of the HERMES (Heuristic Multiqueue Scheduler).
*
* Bujosa, D.; Ashjaei, M.; Papadopoulos, A. V.; Nolte, T. & Proenza, J.
* HERMES: Heuristic Multi-Queue Scheduler for TSN Time-Triggered Traffic with Zero Reception Jitter Capabilities
* Proceedings of the 30th International Conference on Real-Time Networks and Systems, Association for Computing Machinery, 2022, 70–80
*
* Note that we removed the queue limit to fit the overall system model. Therefore, this is not an exact implementation of the HERMES scheduler.
* Additionally, we did not implement the zero-queuing version.
*/

class Hermes final : public solver::AbstractScheduler
{

public:
    explicit Hermes(MultiLayeredGraph& graph);

    auto solve(const MultiLayeredGraph& graph,
               const robin_hood::unordered_set<common::FlowNodeID>& active_f,
               const robin_hood::unordered_set<common::FlowNodeID>& req_f,
               common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet override;

    auto name() -> std::string override;

private:
    auto divPhases() -> std::optional<robin_hood::unordered_map<common::NetworkQueueID, std::size_t>>;

    /**
     * returns the last path segment without a phase assigned or the path segment with the given phase.
     * Note: Hermes assigns phases in reverse order. Therefore the last link in the path without phase is the "next" to get one.
     * @param phases
     * @param path
     * @param phi
     * @return
     */
    static auto getNextPathSegmentForPhase(std::vector<std::size_t>& phases, const std::vector<common::NetworkQueueID>& path, std::size_t phi)
        -> std::optional<common::NetworkQueueID>;

    auto assignFrameUtilization() -> std::vector<float>;

    auto schedule(const robin_hood::unordered_map<common::NetworkQueueID, std::size_t>& phases,
                  const std::vector<float>& frame_utilization,
                  common::NetworkUtilizationList& network_utilization)
        -> solver::solutionSet;

    MultiLayeredGraph& graph_;
};
