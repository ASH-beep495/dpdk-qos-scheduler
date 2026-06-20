// SPDX-License-Identifier: MIT
#pragma once

#include "flow_key.hpp"

#include <array>
#include <chrono>
#include <cstdint>

namespace qos {

class QosQueue;
class Scheduler;

class Stats {
public:
    explicit Stats(uint32_t interval_sec);

    void on_rx(uint32_t bytes);
    void on_tx(PriorityLevel level, uint32_t bytes);
    void on_drop();

    void maybe_print(const std::array<QosQueue, kPriorityCount>& queues, const Scheduler& scheduler);
    void print(const std::array<QosQueue, kPriorityCount>& queues, const Scheduler& scheduler, bool force);

private:
    uint64_t rx_packets_ = 0;
    uint64_t tx_packets_ = 0;
    uint64_t dropped_packets_ = 0;
    uint64_t rx_bytes_ = 0;
    uint64_t tx_bytes_ = 0;
    std::array<uint64_t, kPriorityCount> tx_by_priority_{};

    uint64_t last_tx_packets_ = 0;
    uint64_t last_tx_bytes_ = 0;
    std::chrono::steady_clock::time_point last_print_;
    std::chrono::seconds interval_;
};

} // namespace qos
