// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>

namespace qos {

struct FlowKey {
    uint32_t src_ip = 0;
    uint32_t dst_ip = 0;
    uint16_t src_port = 0;
    uint16_t dst_port = 0;
    uint8_t protocol = 0;
    bool valid = false;

    std::string to_string() const;
};

enum class PriorityLevel : uint8_t {
    High = 0,
    Medium = 1,
    Low = 2,
};

constexpr std::size_t kPriorityCount = 3;

const char* priority_name(PriorityLevel level);
std::size_t priority_index(PriorityLevel level);

} // namespace qos
