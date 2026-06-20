// SPDX-License-Identifier: MIT
#include "sp_scheduler.hpp"

namespace qos {

PriorityLevel StrictPriorityScheduler::select_queue(const QueueViews& queues)
{
    if (!queues[priority_index(PriorityLevel::High)]->empty()) {
        return PriorityLevel::High;
    }
    if (!queues[priority_index(PriorityLevel::Medium)]->empty()) {
        return PriorityLevel::Medium;
    }
    return PriorityLevel::Low;
}

const char* StrictPriorityScheduler::name() const
{
    return "SP";
}

} // namespace qos
