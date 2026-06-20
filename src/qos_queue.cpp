// SPDX-License-Identifier: MIT
#include "qos_queue.hpp"

#include <rte_mbuf.h>

namespace qos {

QosQueue::QosQueue(std::size_t capacity)
    : capacity_(capacity)
{
}

bool QosQueue::enqueue(rte_mbuf* mbuf)
{
    if (full()) {
        return false;
    }
    packets_.push_back(mbuf);
    return true;
}

rte_mbuf* QosQueue::dequeue()
{
    if (packets_.empty()) {
        return nullptr;
    }
    rte_mbuf* mbuf = packets_.front();
    packets_.pop_front();
    return mbuf;
}

rte_mbuf* QosQueue::front() const
{
    if (packets_.empty()) {
        return nullptr;
    }
    return packets_.front();
}

bool QosQueue::empty() const
{
    return packets_.empty();
}

bool QosQueue::full() const
{
    return packets_.size() >= capacity_;
}

std::size_t QosQueue::size() const
{
    return packets_.size();
}

std::size_t QosQueue::capacity() const
{
    return capacity_;
}

uint32_t QosQueue::front_packet_len() const
{
    const rte_mbuf* packet = front();
    if (packet == nullptr) {
        return 0;
    }
    return rte_pktmbuf_pkt_len(packet);
}

bool any_queue_has_packets(const QueueViews& queues)
{
    for (const QosQueue* queue : queues) {
        if (queue != nullptr && !queue->empty()) {
            return true;
        }
    }
    return false;
}

QueueViews make_queue_views(const std::array<QosQueue, kPriorityCount>& queues)
{
    return QueueViews{&queues[0], &queues[1], &queues[2]};
}

} // namespace qos
