#pragma once

#include "Typedefs.h"

class AbstractConfigurationRating
{
public:
    virtual auto prepare() -> void
    {
        // is overwritten by subclasses if they need preparation
    }

    virtual auto rate(common::ConfigurationNodeID config)
        -> float = 0;

    virtual auto toString() -> std::string_view = 0;

    virtual ~AbstractConfigurationRating() = default;
};