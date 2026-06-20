# dpdk-qos-scheduler

`dpdk-qos-scheduler` is a C++17 DPDK-based user-space packet processing and QoS scheduling demo. It demonstrates DPDK EAL initialization, mbuf pool creation, port and RX/TX queue setup, burst packet I/O, Ethernet/IPv4/TCP/UDP parsing, five-tuple classification, multi-priority queues, and three QoS scheduling policies: SP, WRR, and DRR.

This project is intended for learning and technical demonstration. It is not a production-grade switch, router, or gateway.

## Relation to 5G-TSN QoS Scheduling

The 5G-TSN project focuses on QoS mapping and scheduling guarantees for time-sensitive traffic in integrated 5G/TSN scenarios. This project turns a similar idea into a runnable user-space packet processing demo: multiple traffic classes compete for limited forwarding opportunities and are handled by different QoS schedulers.

Demo classification rules:

- UDP destination port `5001` -> `HIGH`, simulating network control traffic.
- UDP destination port `5002` -> `MEDIUM`, simulating video traffic.
- Other IPv4/TCP/UDP or non-IPv4 packets -> `LOW`, simulating best-effort traffic.

These are demo rules only. In a real system, they can be replaced by ACL rules, DSCP, VLAN PCP, five-tuple tables, tunnel metadata, or policies delivered by a control plane.

## Architecture

```text
NIC RX
  -> DPDK rte_eth_rx_burst
  -> PacketParser(Ethernet/IPv4/TCP/UDP)
  -> FlowClassifier(5001/5002/other)
  -> HIGH / MEDIUM / LOW queues
  -> Scheduler(SP / WRR / DRR)
  -> DPDK rte_eth_tx_burst
  -> NIC TX
  -> Stats(PPS, throughput, drops, queue length)
```

## Requirements

- Linux
- DPDK installed with `libdpdk.pc` available
- Meson + Ninja
- A C++17 compiler, such as `g++` or `clang++`
- `pkg-config`

This project depends on DPDK. DPDK follows its official license. This repository only contains personal learning/demo code and is licensed under MIT.

## DPDK Installation Notes

Installation methods vary by distribution. The key requirement is that `pkg-config` can find `libdpdk`:

```bash
pkg-config --modversion libdpdk
pkg-config --cflags --libs libdpdk
```

If you install DPDK from source, a typical workflow is:

```bash
meson setup build
ninja -C build
sudo ninja -C build install
sudo ldconfig
```

If `pkg-config` cannot find DPDK, set `PKG_CONFIG_PATH` according to your installation path, for example:

```bash
export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
```

## HugePages

DPDK uses HugePages to reduce TLB misses and improve user-space packet processing performance. Example:

```bash
sudo ./scripts/setup_hugepages.sh 1024
```

You can also check HugePage status manually:

```bash
grep Huge /proc/meminfo
```

## NIC Binding

For real packet I/O, a NIC usually needs to be bound to a DPDK-compatible driver such as `vfio-pci`. Confirm the PCI address first, then run:

```bash
sudo ./scripts/bind_nic.sh 0000:01:00.0
```

The script is only a teaching example. Binding a NIC may disconnect the host network. Do not bind the management NIC you are currently using.

## Build

```bash
meson setup build
ninja -C build
```

## Dry Run

`dry-run` mode does not require a real DPDK NIC. It creates simulated packets and tests parsing, classification, queueing, and scheduling logic:

```bash
./build/dpdk-qos-scheduler --dry-run
```

It prints enqueue results and scheduling orders for SP, WRR, and DRR.

## Real DPDK Run

When running with a real DPDK port, DPDK EAL options should be placed before `--`, and application options should be placed after `--`:

```bash
sudo ./build/dpdk-qos-scheduler -l 0-1 -n 4 -- \
  --port 0 \
  --scheduler drr \
  --burst-size 32 \
  --queue-size 1024 \
  --stats-interval 1
```

Available schedulers:

```bash
--scheduler sp
--scheduler wrr
--scheduler drr
```

## Scheduling Algorithms

### SP: Strict Priority

Strict Priority always serves the highest non-empty priority queue first:

1. Serve `HIGH` if it is not empty.
2. Otherwise serve `MEDIUM`.
3. Otherwise serve `LOW`.

It provides low latency for high-priority traffic, but low-priority traffic may starve under sustained high-priority load.

### WRR: Weighted Round Robin

Weighted Round Robin serves queues according to configured weights. This demo uses:

```text
HIGH : MEDIUM : LOW = 6 : 3 : 1
```

WRR is fairer than SP, but it usually schedules by packet count. If packet sizes differ significantly, packet-level fairness may not equal byte-level fairness.

### DRR: Deficit Round Robin

Deficit Round Robin maintains a quantum and deficit counter for each queue. It schedules packets based on actual packet length. In this demo, `HIGH` has the largest quantum, `MEDIUM` has a smaller one, and `LOW` has the smallest one.

DRR is more suitable for variable-size packets than WRR because it approximates byte-level fairness while still preserving priority through different quantum values.

## Statistics

The program periodically prints:

- RX packets
- TX packets
- dropped packets
- PPS
- throughput in Mbps
- queue length for HIGH / MEDIUM / LOW
- sent packet count for HIGH / MEDIUM / LOW
- scheduler name

If a queue is full, the packet is dropped and the drop counter is increased.

## Project Boundary

This is a learning and technical demonstration demo:

- It does not implement full routing, switching, ARP, ACL, or a complex control plane.
- It does not guarantee production-grade performance or reliability.
- It does not copy DPDK official example source code.
- DPDK is used as a system dependency and linked through `pkg-config`.
- The main goal is to understand the DPDK packet processing path and QoS scheduling algorithms.
