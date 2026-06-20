// SPDX-License-Identifier: MIT
#include "drr_scheduler.hpp"

#include <rte_mbuf.h>

namespace qos {

DeficitRoundRobinScheduler::DeficitRoundRobinScheduler()
    : quantum_{1536 * 6, 1536 * 3, 1536}, deficit_{0, 0, 0}
{
}

PriorityLevel DeficitRoundRobinScheduler::select_queue(const QueueViews& queues)
{
    constexpr std::size_t max_rounds = kPriorityCount * 4;

    for (std::size_t round = 0; round < max_rounds; ++round) {
        PriorityLevel candidate = static_cast<PriorityLevel>(cursor_);
        const std::size_t index = priority_index(candidate);
        const QosQueue* queue = queues[index];

        if (queue != nullptr && !queue->empty()) {
            deficit_[index] += quantum_[index];
            if (queue->front_packet_len() <= deficit_[index]) {
                return candidate;
            }
        }

        cursor_ = next_index(cursor_);
    }

    for (std::size_t index = 0; index < kPriorityCount; ++index) {
        if (queues[index] != nullptr && !queues[index]->empty()) {
            return static_cast<PriorityLevel>(index);
        }
    }
    return PriorityLevel::Low;
}

void DeficitRoundRobinScheduler::on_packet_sent(PriorityLevel level, const rte_mbuf* mbuf)
{
    const std::size_t index = priority_index(level);
    const uint32_t packet_len = mbuf == nullptr ? 0 : rte_pktmbuf_pkt_len(mbuf);
    if (deficit_[index] >= packet_len) {
        deficit_[index] -= packet_len;
    } else {
        deficit_[index] = 0;
    }
    cursor_ = next_index(index);
}

const char* DeficitRoundRobinScheduler::name() const
{
    return "DRR";
}

std::size_t DeficitRoundRobinScheduler::next_index(std::size_t index)
{
    return (index + 1) % kPriorityCount;
}

} // namespace qos
