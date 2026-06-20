// SPDX-License-Identifier: MIT
#pragma once

#include "config.hpp"

#include <cstdint>

struct rte_mbuf;
struct rte_mempool;

namespace qos {

class DpdkPort {
public:
    explicit DpdkPort(const AppConfig& config);
    ~DpdkPort();

    bool init();
    uint16_t rx_burst(rte_mbuf** packets, uint16_t count);
    uint16_t tx_burst(rte_mbuf** packets, uint16_t count);
    void close();

private:
    AppConfig config_;
    rte_mempool* mempool_ = nullptr;
    bool started_ = false;

    bool configure_port();
};

} // namespace qos
