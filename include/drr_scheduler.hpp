// SPDX-License-Identifier: MIT
#pragma once

#include "scheduler.hpp"

#include <array>
#include <cstdint>

namespace qos {

class DeficitRoundRobinScheduler final : public Scheduler {
public:
    DeficitRoundRobinScheduler();

    PriorityLevel select_queue(const QueueViews& queues) override;
    void on_packet_sent(PriorityLevel level, const rte_mbuf* mbuf) override;
    const char* name() const override;

private:
    std::array<uint32_t, kPriorityCount> quantum_{};
    std::array<uint32_t, kPriorityCount> deficit_{};
    std::size_t cursor_ = 0;

    static std::size_t next_index(std::size_t index);
};

} // namespace qos
