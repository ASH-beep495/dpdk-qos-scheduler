# SPDX-License-Identifier: MIT
# Interview Notes

## Short Pitch

This project is a C++17 DPDK packet-processing Demo with QoS scheduling. It initializes DPDK, configures an RX/TX port, parses Ethernet/IPv4/TCP/UDP packets, extracts the five-tuple, classifies traffic into high/medium/low priority queues, and schedules packets with SP, WRR, or DRR.

## Why DPDK

DPDK avoids the traditional kernel socket path for high-rate packet processing. It uses user-space polling, hugepages, mbufs, burst APIs, and NIC queue binding to reduce syscall and copy overhead.

## Relation to 5G-TSN

The 5G-TSN project focused on QoS mapping and scheduling in simulation. This DPDK project turns that idea into a runnable packet-processing demo where different traffic classes compete for limited forwarding opportunities.

## Boundary

This is not a production router or switch. It is a learning project for understanding DPDK RX/TX, packet parsing, flow classification, queueing, and QoS scheduling.
