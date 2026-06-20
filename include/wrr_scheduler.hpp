// SPDX-License-Identifier: MIT
#pragma once

#include "scheduler.hpp"

#include <array>
#include <vector>

namespace qos {

class WeightedRoundRobinScheduler final : public Scheduler {
public:
    WeightedRoundRobinScheduler();

    PriorityLevel select_queue(const QueueViews& queues) override;
    const char* name() const override;

private:
    std::vector<PriorityLevel> schedule_ring_;
    std::size_t cursor_ = 0;
};

} // namespace qos
