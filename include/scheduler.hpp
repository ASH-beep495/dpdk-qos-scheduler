// SPDX-License-Identifier: MIT
#pragma once

#include "flow_key.hpp"
#include "qos_queue.hpp"

struct rte_mbuf;

namespace qos {

class Scheduler {
public:
    virtual PriorityLevel select_queue(const QueueViews& queues) = 0;
    virtual void on_packet_sent(PriorityLevel level, const rte_mbuf* mbuf);
    virtual const char* name() const = 0;
    virtual ~Scheduler() = default;
};

} // namespace qos
