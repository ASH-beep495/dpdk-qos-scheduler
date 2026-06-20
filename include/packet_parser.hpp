// SPDX-License-Identifier: MIT
#pragma once

#include "flow_key.hpp"

#include <optional>

struct rte_mbuf;

namespace qos {

struct ParsedPacket {
    FlowKey flow;
    bool is_ipv4 = false;
    bool is_tcp = false;
    bool is_udp = false;
};

class PacketParser {
public:
    std::optional<ParsedPacket> parse(const rte_mbuf* mbuf) const;
};

} // namespace qos
