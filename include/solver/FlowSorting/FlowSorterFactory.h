#pragma once

#include "AbstractFlowSorter.h"
#include "graph/MultiLayeredGraph.h"


namespace flow_sorting {

enum class FlowSorterTypes {
    HIGHEST_TRAFFIC_FLOWS_FIRST = 0,
    LOWEST_TRAFFIC_FLOWS_FIRST = 1,
    LOWEST_ID_FIRST = 2,
    SOURCE_NODE_SORTING = 3,
    LOW_PERIOD_FLOWS_FIRST = 4
};

[[nodiscard]] inline auto to_int(FlowSorterTypes type) -> int
{
    return static_cast<int>(type);
}

[[nodiscard]] auto get_flow_sorter(MultiLayeredGraph& graph, FlowSorterTypes type) -> std::unique_ptr<AbstractFlowSorter>;

} // namespace flow_sorting