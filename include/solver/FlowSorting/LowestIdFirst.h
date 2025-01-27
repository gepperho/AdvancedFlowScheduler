#pragma once

class LowestIdFirst final : public AbstractFlowSorter {

public:

    auto toString() -> std::string_view override {
        return "Low-IDs-First";
    }

    auto operator()(const common::FlowNodeID lhs, const common::FlowNodeID rhs) const -> bool override
    {
        return lhs > rhs;
    }
};