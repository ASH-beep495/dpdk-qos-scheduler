// SPDX-License-Identifier: MIT
#pragma once

#include "scheduler.hpp"

namespace qos {

class StrictPriorityScheduler final : public Scheduler {
public:
    PriorityLevel select_queue(const QueueViews& queues) override;
    const char* name() const override;
};

} // namespace qos
