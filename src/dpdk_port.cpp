// SPDX-License-Identifier: MIT
#include "dpdk_port.hpp"

#include <iostream>

#include <rte_ethdev.h>
#include <rte_mbuf.h>

namespace qos {

namespace {
constexpr uint16_t kRxRingSize = 1024;
constexpr uint16_t kTxRingSize = 1024;
constexpr uint32_t kNumMbufs = 8191;
constexpr uint32_t kMbufCacheSize = 250;
constexpr uint16_t kRxQueueId = 0;
constexpr uint16_t kTxQueueId = 0;
} // namespace

DpdkPort::DpdkPort(const AppConfig& config)
    : config_(config)
{
}

DpdkPort::~DpdkPort()
{
    close();
}

bool DpdkPort::init()
{
    mempool_ = rte_pktmbuf_pool_create("qos_mbuf_pool", kNumMbufs, kMbufCacheSize, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (mempool_ == nullptr) {
        std::cerr << "Failed to create mbuf pool\n";
        return false;
    }
    return configure_port();
}

uint16_t DpdkPort::rx_burst(rte_mbuf** packets, uint16_t count)
{
    return rte_eth_rx_burst(config_.port_id, kRxQueueId, packets, count);
}

uint16_t DpdkPort::tx_burst(rte_mbuf** packets, uint16_t count)
{
    return rte_eth_tx_burst(config_.port_id, kTxQueueId, packets, count);
}

void DpdkPort::close()
{
    if (started_) {
        rte_eth_dev_stop(config_.port_id);
        rte_eth_dev_close(config_.port_id);
        started_ = false;
    }
}

bool DpdkPort::configure_port()
{
    if (!rte_eth_dev_is_valid_port(config_.port_id)) {
        std::cerr << "Invalid DPDK port id: " << config_.port_id << '\n';
        return false;
    }

    rte_eth_conf port_conf{};
    port_conf.rxmode.mq_mode = RTE_ETH_MQ_RX_NONE;
    port_conf.txmode.mq_mode = RTE_ETH_MQ_TX_NONE;

    int ret = rte_eth_dev_configure(config_.port_id, 1, 1, &port_conf);
    if (ret < 0) {
        std::cerr << "rte_eth_dev_configure failed: " << ret << '\n';
        return false;
    }

    ret = rte_eth_rx_queue_setup(config_.port_id, kRxQueueId, kRxRingSize, rte_eth_dev_socket_id(config_.port_id), nullptr, mempool_);
    if (ret < 0) {
        std::cerr << "rte_eth_rx_queue_setup failed: " << ret << '\n';
        return false;
    }

    ret = rte_eth_tx_queue_setup(config_.port_id, kTxQueueId, kTxRingSize, rte_eth_dev_socket_id(config_.port_id), nullptr);
    if (ret < 0) {
        std::cerr << "rte_eth_tx_queue_setup failed: " << ret << '\n';
        return false;
    }

    ret = rte_eth_dev_start(config_.port_id);
    if (ret < 0) {
        std::cerr << "rte_eth_dev_start failed: " << ret << '\n';
        return false;
    }

    started_ = true;
    ret = rte_eth_promiscuous_enable(config_.port_id);
    if (ret < 0) {
        std::cerr << "Failed to enable promiscuous mode: " << ret << '\n';
        return false;
    }

    rte_ether_addr mac{};
    rte_eth_macaddr_get(config_.port_id, &mac);
    char mac_buffer[RTE_ETHER_ADDR_FMT_SIZE]{};
    rte_ether_format_addr(mac_buffer, sizeof(mac_buffer), &mac);
    std::cout << "Initialized DPDK port " << config_.port_id << " mac=" << mac_buffer << '\n';
    return true;
}

} // namespace qos
