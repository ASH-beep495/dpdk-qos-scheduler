# SPDX-License-Identifier: MIT
# Design

## Goal

`dpdk-qos-scheduler` is a small teaching-oriented DPDK application. It focuses on the full loop from RX burst to packet parsing, flow classification, priority queues, scheduling, TX burst, and stats.

## Data Path

```text
rte_eth_rx_burst
  -> PacketParser
  -> FlowClassifier
  -> QosQueue[HIGH/MEDIUM/LOW]
  -> Scheduler
  -> rte_eth_tx_burst
```

## Modules

- `DpdkPort`: EAL-independent port setup wrapper for mempool, RX/TX queue, promiscuous mode, RX/TX burst.
- `PacketParser`: extracts Ethernet, optional VLAN, IPv4, TCP/UDP, and five-tuple fields.
- `FlowClassifier`: maps Demo traffic rules into `PriorityLevel`.
- `QosQueue`: bounded FIFO storing `rte_mbuf*`.
- `Scheduler`: common interface for SP, WRR, DRR.
- `Stats`: counts packets, bytes, drops, queue lengths, PPS, throughput.

## Demo Classification Rules

```text
UDP dst port 5001 -> HIGH
UDP dst port 5002 -> MEDIUM
Other IPv4 TCP/UDP -> LOW
Non-IPv4 -> LOW
```

## Scheduler Behavior

- SP minimizes high-priority latency, but low priority can starve.
- WRR gives a packet-count service ratio of 6:3:1.
- DRR uses byte-oriented deficit counters and is more robust with variable packet sizes.

## Limitations

This project intentionally does not implement production features such as ARP, routing, multi-core scaling, NIC offload tuning, lock-free rings, telemetry, control-plane policy loading, or zero-loss shutdown.
