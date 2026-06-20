// SPDX-License-Identifier: MIT
#include "stats.hpp"

#include "qos_queue.hpp"
#include "scheduler.hpp"

#include <iomanip>
#include <iostream>

namespace qos {

Stats::Stats(uint32_t interval_sec)
    : last_print_(std::chrono::steady_clock::now()), interval_(std::chrono::seconds(interval_sec == 0 ? 1 : interval_sec))
{
}

void Stats::on_rx(uint32_t bytes)
{
    ++rx_packets_;
    rx_bytes_ += bytes;
}

void Stats::on_tx(PriorityLevel level, uint32_t bytes)
{
    ++tx_packets_;
    tx_bytes_ += bytes;
    ++tx_by_priority_[priority_index(level)];
}

void Stats::on_drop()
{
    ++dropped_packets_;
}

void Stats::maybe_print(const std::array<QosQueue, kPriorityCount>& queues, const Scheduler& scheduler)
{
    print(queues, scheduler, false);
}

void Stats::print(const std::array<QosQueue, kPriorityCount>& queues, const Scheduler& scheduler, bool force)
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = now - last_print_;
    if (!force && elapsed < interval_) {
        return;
    }

    const double seconds = std::chrono::duration<double>(elapsed).count();
    const uint64_t delta_packets = tx_packets_ - last_tx_packets_;
    const uint64_t delta_bytes = tx_bytes_ - last_tx_bytes_;
    const double pps = seconds > 0 ? static_cast<double>(delta_packets) / seconds : 0.0;
    const double mbps = seconds > 0 ? (static_cast<double>(delta_bytes) * 8.0) / seconds / 1'000'000.0 : 0.0;

    std::cout << "[stats] scheduler=" << scheduler.name()
              << " rx=" << rx_packets_
              << " tx=" << tx_packets_
              << " drop=" << dropped_packets_
              << " pps=" << std::fixed << std::setprecision(2) << pps
              << " throughput=" << mbps << " Mbps"
              << " q_high=" << queues[priority_index(PriorityLevel::High)].size()
              << " q_medium=" << queues[priority_index(PriorityLevel::Medium)].size()
              << " q_low=" << queues[priority_index(PriorityLevel::Low)].size()
              << " tx_high=" << tx_by_priority_[priority_index(PriorityLevel::High)]
              << " tx_medium=" << tx_by_priority_[priority_index(PriorityLevel::Medium)]
              << " tx_low=" << tx_by_priority_[priority_index(PriorityLevel::Low)]
              << '\n';

    last_print_ = now;
    last_tx_packets_ = tx_packets_;
    last_tx_bytes_ = tx_bytes_;
}

} // namespace qos
