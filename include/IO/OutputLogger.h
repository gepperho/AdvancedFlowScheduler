#pragma once
#include "MetaDataLog.h"
#include <fmt/core.h>

namespace io {

class OutputLogger
{

public:
    static auto getInstance() -> OutputLogger&
    {
        static OutputLogger instance;
        return instance;
    }

private:
    // Private Constructor
    OutputLogger() = default;

public:
    // Stop the compiler generating methods of copy the object
    OutputLogger(OutputLogger const& copy) = delete; // Not Implemented
    auto operator=(OutputLogger const& copy) -> OutputLogger& = delete; // Not Implemented


    auto printLog() -> void
    {
        for(const auto& log_entry : logs_) {
            fmt::print(

                "Time step: {}\n"
                "Network: {}\n"
                "Scenario: {}\n"
                "Routing: {}\n"
                "Candidate Routes: {}\n"
                "Strategy: {}\n"
                "Config Placement: {}\n"
                "{}\n"
                "============================\n",
                log_entry.time_step,
                network_,
                scenario_,
                routing_,
                no_candidate_routes_,
                strategy_,
                to_int(placement_),
                meta_data_log_to_pretty_string(log_entry));
        }
    }

    auto printRawLog() -> void
    {
        for(const auto& log_entry : logs_) {
            fmt::print("{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\n",
                       log_entry.time_step,
                       network_,
                       scenario_,
                       routing_,
                       no_candidate_routes_,
                       strategy_,
                       placement::to_int(placement_),
                       io::meta_data_log_to_raw_string(log_entry));
        }
    }

    auto addLog(MetaDataLog& log) -> void
    {
        logs_.emplace_back(log);
    }

    auto setStrategy(const std::string_view strategy) -> void
    {
        strategy_ = strategy;
    }

    auto setNetwork(const std::string_view network) -> void
    {
        network_ = network;
    }

    auto setScenario(const std::string_view scenario) -> void
    {
        scenario_ = scenario;
    }

    auto setRouting(const std::string_view routing) -> void
    {
        routing_ = routing;
    }

    auto setPlacement(const placement::ConfigPlacementTypes placement) -> void
    {
        placement_ = placement;
    }

    auto setNoCandidateRoutes(const std::size_t no_candidate_routes) -> void
    {
        no_candidate_routes_ = no_candidate_routes;
    }


private:
    std::vector<MetaDataLog> logs_;
    std::string strategy_;
    std::string network_;
    std::string scenario_;
    std::string routing_;
    std::size_t no_candidate_routes_;
    placement::ConfigPlacementTypes placement_;
};

} // namespace io
