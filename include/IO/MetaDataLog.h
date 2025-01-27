#pragma once

namespace io {

struct MetaDataLog
{
    std::size_t flows_scheduled;
    std::size_t flows_total;
    double config_time;
    double solving_time;
    std::size_t time_step;
    std::string planning_mode;
    double traffic;
    std::size_t number_of_frames;
    std::size_t max_queue_size;
    float avg_scheduling_table_size;
    std::size_t max_scheduling_table_size;
};

/**
 * Returns a raw string with most of the MetaDataLog values.
 * Note that the time_step value is not part of the string!
 * The string does NOT end with a new line.
 */
inline auto meta_data_log_to_raw_string(const MetaDataLog &log)
{
    return fmt::format("{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}\t{}",
                       log.planning_mode,
                       log.flows_scheduled,
                       log.flows_total,
                       log.traffic,
                       log.number_of_frames,
                       log.solving_time,
                       log.config_time,
                       log.max_queue_size,
                       log.avg_scheduling_table_size,
                       log.max_scheduling_table_size);
}

/**
 * Returns a pretty string with most of the MetaDataLog values.
 * Note that the time_step value is not part of the string!
 * The string does NOT end with a new line.
 * @param log
 * @return
 */
inline auto meta_data_log_to_pretty_string(const MetaDataLog &log) -> std::string
{
    return fmt::format(
        "Mode: {}\n"
        "Flows scheduled: {}\n"
        "Flows total: {}\n"
        "Ingress traffic [M bit/s]: {}\n"
        "Number of scheduled frames: {}\n"
        "Solving time [s]: {}\n"
        "Configuration time [s]: {}\n"
        "Maximum queue size required: {}\n"
        "Average scheduling table length: {}\n"
        "Maximum scheduling table length: {}",
        log.planning_mode,
        log.flows_scheduled,
        log.flows_total,
        log.traffic,
        log.number_of_frames,
        log.solving_time,
        log.config_time,
        log.max_queue_size,
        log.avg_scheduling_table_size,
        log.max_scheduling_table_size);
}

} // namespace io