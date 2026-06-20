// SPDX-License-Identifier: MIT
#include "wrr_scheduler.hpp"

namespace qos {

WeightedRoundRobinScheduler::WeightedRoundRobinScheduler()
{
    schedule_ring_.reserve(10);
    for (int i = 0; i < 6; ++i) {
        schedule_ring_.push_back(PriorityLevel::High);
    }
    for (int i = 0; i < 3; ++i) {
        schedule_ring_.push_back(PriorityLevel::Medium);
    }
    schedule_ring_.push_back(PriorityLevel::Low);
}

PriorityLevel WeightedRoundRobinScheduler::select_queue(const QueueViews& queues)
{
    for (std::size_t attempts = 0; attempts < schedule_ring_.size(); ++attempts) {
        PriorityLevel candidate = schedule_ring_[cursor_];
        cursor_ = (cursor_ + 1) % schedule_ring_.size();
        if (!queues[priority_index(candidate)]->empty()) {
            return candidate;
        }
    }
    return PriorityLevel::Low;
}

const char* WeightedRoundRobinScheduler::name() const
{
    return "WRR";
}

} // namespace qos
