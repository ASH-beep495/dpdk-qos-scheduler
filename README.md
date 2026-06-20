# dpdk-qos-scheduler

`dpdk-qos-scheduler` 是一个基于 DPDK 的用户态高速包处理与 QoS 流量调度 Demo。项目使用 C++17 编写，重点演示 DPDK EAL 初始化、mempool 创建、端口 RX/TX 队列配置、burst 收发包、Ethernet/IPv4/TCP/UDP 解析、五元组分类、多级队列和 SP/WRR/DRR 三种调度策略。

本项目是个人学习和技术演示项目，不是生产级交换机、路由器或完整网关。

## 与 5G-TSN QoS 调度理解的关系

5G-TSN 项目关注时间敏感业务在 5G/TSN 融合场景下的 QoS 映射和调度保障。本项目把这种“多业务优先级 + 队列 + 调度算法 + 指标统计”的思想落到用户态包处理 Demo 中：

- UDP 目的端口 `5001` 映射为 `HIGH`，模拟 network control。
- UDP 目的端口 `5002` 映射为 `MEDIUM`，模拟 video。
- 其他 IPv4/TCP/UDP 或非 IPv4 报文映射为 `LOW`，模拟 best effort。

这些规则只是 Demo 规则，真实项目可以替换为 ACL、DSCP、VLAN PCP、五元组表、隧道字段或控制面下发的策略表。

## 架构

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

## 环境依赖

- Linux
- DPDK，已安装并提供 `libdpdk.pc`
- Meson + Ninja
- C++17 编译器，例如 `g++` 或 `clang++`
- `pkg-config`

本项目依赖 DPDK，DPDK 本身遵循其官方许可证；本仓库只包含个人学习项目代码，许可证为 MIT。

## DPDK 安装说明

不同发行版安装方式不同，核心目标是让系统能找到 `libdpdk.pc`：

```bash
pkg-config --modversion libdpdk
pkg-config --cflags --libs libdpdk
```

如果你从源码安装 DPDK，通常需要在 DPDK 源码目录中执行类似流程：

```bash
meson setup build
ninja -C build
sudo ninja -C build install
sudo ldconfig
```

如果 `pkg-config` 找不到 DPDK，可设置：

```bash
export PKG_CONFIG_PATH=/usr/local/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
```

具体路径以你的 DPDK 安装位置为准。

## HugePages

DPDK 使用 HugePages 减少 TLB miss，提高用户态数据包处理性能。示例脚本：

```bash
sudo ./scripts/setup_hugepages.sh 1024
```

也可以手动查看：

```bash
grep Huge /proc/meminfo
```

## 网卡绑定

真实收发包通常需要把网卡绑定到 DPDK 支持的驱动，例如 `vfio-pci`。先确认网卡 PCI 地址，再执行：

```bash
sudo ./scripts/bind_nic.sh 0000:01:00.0
```

脚本只是教学示例，绑定网卡会影响系统网络连接，请确认不要绑定正在使用的管理网卡。

## 构建

```bash
meson setup build
ninja -C build
```

## dry-run 运行

`dry-run` 不依赖真实 DPDK 网卡收发，用模拟报文测试解析、分类和调度逻辑，适合无网卡环境演示：

```bash
./build/dpdk-qos-scheduler --dry-run
```

它会分别打印 SP、WRR、DRR 的入队分类和调度输出顺序。

## 真实 DPDK 运行

真实运行时需要 EAL 参数和 app 参数。DPDK EAL 参数放在 `--` 前，应用参数放在 `--` 后：

```bash
sudo ./build/dpdk-qos-scheduler -l 0-1 -n 4 -- \
  --port 0 \
  --scheduler drr \
  --burst-size 32 \
  --queue-size 1024 \
  --stats-interval 1
```

可选调度器：

```bash
--scheduler sp
--scheduler wrr
--scheduler drr
```

## 三种调度算法

### SP，Strict Priority

严格优先级调度。只要 `HIGH` 非空就发 `HIGH`，否则发 `MEDIUM`，最后才发 `LOW`。

优点是高优先级业务时延低；缺点是高优先级持续满载时，低优先级可能饥饿。

### WRR，Weighted Round Robin

加权轮询。Demo 权重为：

```text
HIGH : MEDIUM : LOW = 6 : 3 : 1
```

优点是比 SP 更公平；缺点是它按“包数量”轮转，包长变化明显时，字节级公平性不足。

### DRR，Deficit Round Robin

赤字轮询。每个队列有 `quantum` 和 `deficit counter`，调度时按包长扣减 deficit。Demo 中 `HIGH` quantum 最大，`MEDIUM` 次之，`LOW` 最小。

优点是比 WRR 更适合变长包，因为它按字节长度近似公平；同时还能通过 quantum 保留业务优先级。

## 统计指标

程序周期打印：

- RX packets
- TX packets
- dropped packets
- PPS
- throughput Mbps
- HIGH/MEDIUM/LOW 队列长度
- HIGH/MEDIUM/LOW 发送数量
- 当前调度算法名称

队列满时，报文会被丢弃并增加 drop counter。

## 项目边界

本项目是个人学习和技术演示 Demo：

- 不实现完整路由、交换、ARP、ACL 或复杂控制面。
- 不保证生产级性能或可靠性。
- 不复制 DPDK 官方 examples 源码。
- DPDK 作为系统依赖，通过 `pkg-config` 链接。
- 核心价值是帮助理解 DPDK 包处理路径和 QoS 调度算法。
