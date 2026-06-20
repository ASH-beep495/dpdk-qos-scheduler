# SPDX-License-Identifier: MIT
# Test Plan

## 1. Dry Run

Goal: verify parser, classifier, queues, and scheduler output without a DPDK NIC.

```bash
./build/dpdk-qos-scheduler --dry-run
```

Expected:

- UDP/5001 packets are classified as HIGH.
- UDP/5002 packets are classified as MEDIUM.
- Other packets are classified as LOW.
- SP drains HIGH before MEDIUM before LOW.
- WRR roughly follows 6:3:1 packet service.
- DRR considers packet length through deficit counters.

## 2. Single-Port Loopback

Goal: verify real RX/TX path with one port and external loopback cable or test equipment.

Steps:

1. Configure HugePages.
2. Bind NIC to a DPDK-compatible driver.
3. Connect TX/RX path through loopback or a tester.
4. Run:

```bash
sudo ./build/dpdk-qos-scheduler -l 0-1 -n 4 -- --port 0 --scheduler drr
```

Observe stats for RX/TX counters and drops.

## 3. Dual-Port Forwarding Extension

Current code uses one port for RX/TX. A simple extension is to add `--rx-port` and `--tx-port`, create two `DpdkPort` objects, RX from one and TX to the other.

Test with two NIC ports connected through a traffic generator.

## 4. iperf / pktgen Idea

- Use `pktgen-dpdk` or a hardware tester to generate UDP flows with destination ports 5001, 5002, and other ports.
- Compare SP/WRR/DRR stats under overload.
- Check queue length and drop distribution.

## 5. Metrics

- PPS: transmitted packets per second.
- Throughput: transmitted bytes per second converted to Mbps.
- Drops: packets dropped due to full queues or failed TX.
- Queue length: current backlog for HIGH/MEDIUM/LOW.
- Sent count per priority: confirms scheduler behavior.
