#pragma once

#include <Typedefs.h>

class AbstractFlowSorter
{
public:
    virtual auto operator()(common::FlowNodeID lhs, common::FlowNodeID rhs) const -> bool = 0;

    [[nodiscard]] virtual auto toString() -> std::string_view = 0;

    virtual ~AbstractFlowSorter() = default;
};
