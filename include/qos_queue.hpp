// SPDX-License-Identifier: MIT
#pragma once

#include "flow_key.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>

struct rte_mbuf;

namespace qos {

class QosQueue {
public:
    explicit QosQueue(std::size_t capacity = 1024);

    bool enqueue(rte_mbuf* mbuf);
    rte_mbuf* dequeue();
    rte_mbuf* front() const;

    bool empty() const;
    bool full() const;
    std::size_t size() const;
    std::size_t capacity() const;
    uint32_t front_packet_len() const;

private:
    std::size_t capacity_;
    std::deque<rte_mbuf*> packets_;
};

using QueueViews = std::array<const QosQueue*, kPriorityCount>;

bool any_queue_has_packets(const QueueViews& queues);
QueueViews make_queue_views(const std::array<QosQueue, kPriorityCount>& queues);

} // namespace qos
