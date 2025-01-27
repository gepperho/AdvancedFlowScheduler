#pragma once

#include "solver/FlowSorting/AbstractFlowSorter.h"

class LowPeriodFlowsFirst final : public AbstractFlowSorter
{
public:
    explicit LowPeriodFlowsFirst(MultiLayeredGraph& graph)
        : graph_(graph)
    {}

    auto toString() -> std::string_view override
    {
        return "Low-Period-Flows-First";
    }

    auto operator()(const common::FlowNodeID lhs, const common::FlowNodeID rhs) const -> bool override
    {
        const auto& lhs_flow = graph_.getFlow(lhs);
        const auto& rhs_flow = graph_.getFlow(rhs);
        if(lhs_flow.period != rhs_flow.period) {
            // small periods
            return lhs_flow.period > rhs_flow.period;
        }
        if(lhs_flow.frame_size != rhs_flow.frame_size) {
            // long transmissions
            return lhs_flow.frame_size < rhs_flow.frame_size;
        }
        return lhs_flow.id > rhs_flow.id;
    }


private:
    MultiLayeredGraph& graph_;
};