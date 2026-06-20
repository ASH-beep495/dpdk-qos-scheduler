// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace qos {

enum class SchedulerType {
    StrictPriority,
    WeightedRoundRobin,
    DeficitRoundRobin,
};

struct AppConfig {
    uint16_t port_id = 0;
    SchedulerType scheduler = SchedulerType::StrictPriority;
    uint16_t burst_size = 32;
    std::size_t queue_size = 1024;
    uint32_t stats_interval_sec = 1;
    bool dry_run = false;
};

SchedulerType parse_scheduler_type(const std::string& value);
const char* scheduler_type_name(SchedulerType type);

} // namespace qos
