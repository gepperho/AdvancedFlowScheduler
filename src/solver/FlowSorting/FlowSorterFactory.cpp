#include "solver/FlowSorting/FlowSorterFactory.h"
#include "solver/FlowSorting/HighTrafficFlowsFirst.h"
#include "solver/FlowSorting/LowPeriodFlowsFirst.h"
#include "solver/FlowSorting/LowTrafficFlowsFirst.h"
#include "solver/FlowSorting/LowestIdFirst.h"
#include "solver/FlowSorting/SourceNodeSorting.h"

auto flow_sorting::get_flow_sorter(MultiLayeredGraph& graph, const FlowSorterTypes type) -> std::unique_ptr<AbstractFlowSorter>
{
    switch(type) {
        using enum FlowSorterTypes;
    case HIGHEST_TRAFFIC_FLOWS_FIRST:
        return std::make_unique<HighTrafficFlowsFirst>(graph);
    case LOWEST_TRAFFIC_FLOWS_FIRST:
        return std::make_unique<LowTrafficFlowsFirst>(graph);
    case SOURCE_NODE_SORTING:
        return std::make_unique<SourceNodeSorting>(graph);
    case LOW_PERIOD_FLOWS_FIRST:
        return std::make_unique<LowPeriodFlowsFirst>(graph);
    default:
        return std::make_unique<LowestIdFirst>();
    }
}
