#pragma once

#include <NamedType/named_type.hpp>

namespace common {

using NetworkNodeID = fluent::NamedType<std::size_t, struct NetworkNodeIDTag, fluent::Hashable, fluent::Comparable, fluent::Addable, fluent::Multiplicable, fluent::Incrementable>;

using NetworkQueueID = fluent::NamedType<std::size_t, struct NetworkQueueIDTag, fluent::Hashable, fluent::Comparable, fluent::Addable, fluent::Multiplicable, fluent::Incrementable>;

using ConfigurationNodeID = fluent::NamedType<std::size_t, struct ConfigurationNodeIDTag, fluent::Hashable, fluent::Comparable, fluent::Addable, fluent::Multiplicable, fluent::Incrementable>;

using FlowNodeID = fluent::NamedType<std::size_t, struct FlowNodeIDTag, fluent::Hashable, fluent::Comparable, fluent::Addable, fluent::Multiplicable, fluent::Incrementable>;

} // namespace common
