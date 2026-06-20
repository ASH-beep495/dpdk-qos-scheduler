// SPDX-License-Identifier: MIT
#pragma once

#include "flow_key.hpp"
#include "packet_parser.hpp"

namespace qos {

class FlowClassifier {
public:
    PriorityLevel classify(const std::optional<ParsedPacket>& packet) const;
};

} // namespace qos
