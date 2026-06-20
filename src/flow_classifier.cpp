// SPDX-License-Identifier: MIT
#include "flow_classifier.hpp"

#include <rte_ip.h>

namespace qos {

PriorityLevel FlowClassifier::classify(const std::optional<ParsedPacket>& packet) const
{
    if (!packet || !packet->flow.valid || !packet->is_ipv4) {
        return PriorityLevel::Low;
    }

    const FlowKey& flow = packet->flow;
    if (flow.protocol == IPPROTO_UDP && flow.dst_port == 5001) {
        return PriorityLevel::High;
    }
    if (flow.protocol == IPPROTO_UDP && flow.dst_port == 5002) {
        return PriorityLevel::Medium;
    }
    return PriorityLevel::Low;
}

} // namespace qos
