#pragma once

#include "Typedefs.h"
#include "graph/MultiLayeredGraph.h"

class AbstractCelfRating
{
public:
    virtual auto prepare() -> void
    {
        // is overwritten by subclasses if they need preparation
    }

    virtual auto rate(common::ConfigurationNodeID config)
        -> std::pair<float, float> = 0;

    virtual auto pick(common::ConfigurationNodeID config) -> void {
        // is overwritten in subclasses if needed
    }

    virtual auto toString() -> std::string_view = 0;

    virtual ~AbstractCelfRating() = default;
};