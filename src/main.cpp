// SPDX-License-Identifier: MIT
#include "config.hpp"
#include "dpdk_port.hpp"
#include "drr_scheduler.hpp"
#include "flow_classifier.hpp"
#include "packet_parser.hpp"
#include "qos_queue.hpp"
#include "scheduler.hpp"
#include "sp_scheduler.hpp"
#include "stats.hpp"
#include "wrr_scheduler.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <rte_eal.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <rte_udp.h>

namespace qos {

namespace {

struct SimulatedPacket {
    rte_mbuf mbuf{};
    std::vector<uint8_t> storage;
    std::string label;
};

std::unique_ptr<Scheduler> make_scheduler(SchedulerType type)
{
    switch (type) {
    case SchedulerType::StrictPriority:
        return std::make_unique<StrictPriorityScheduler>();
    case SchedulerType::WeightedRoundRobin:
        return std::make_unique<WeightedRoundRobinScheduler>();
    case SchedulerType::DeficitRoundRobin:
        return std::make_unique<DeficitRoundRobinScheduler>();
    }
    throw std::invalid_argument("unknown scheduler type");
}

void print_usage(const char* program)
{
    std::cout << "Usage:\n"
              << "  " << program << " [EAL options] -- [app options]\n\n"
              << "App options:\n"
              << "  --port <id>                 DPDK port id, default 0\n"
              << "  --scheduler sp|wrr|drr      QoS scheduler, default sp\n"
              << "  --burst-size <n>            RX/TX burst size, default 32\n"
              << "  --queue-size <n>            Per-priority queue size, default 1024\n"
              << "  --stats-interval <sec>      Stats print interval, default 1\n"
              << "  --dry-run                   Run parser/classifier/scheduler demo without NIC\n"
              << "  --help                      Show help\n";
}

uint32_t parse_u32(const char* value, const char* name)
{
    try {
        return static_cast<uint32_t>(std::stoul(value));
    } catch (const std::exception&) {
        throw std::invalid_argument(std::string("invalid value for ") + name + ": " + value);
    }
}

AppConfig parse_app_args(int argc, char** argv, int start_index = 1)
{
    AppConfig config{};
    for (int i = start_index; i < argc; ++i) {
        const std::string arg = argv[i];
        auto need_value = [&](const char* name) -> const char* {
            if (i + 1 >= argc) {
                throw std::invalid_argument(std::string("missing value for ") + name);
            }
            return argv[++i];
        };

        if (arg == "--port") {
            config.port_id = static_cast<uint16_t>(parse_u32(need_value("--port"), "--port"));
        } else if (arg == "--scheduler") {
            config.scheduler = parse_scheduler_type(need_value("--scheduler"));
        } else if (arg == "--burst-size") {
            config.burst_size = static_cast<uint16_t>(parse_u32(need_value("--burst-size"), "--burst-size"));
        } else if (arg == "--queue-size") {
            config.queue_size = parse_u32(need_value("--queue-size"), "--queue-size");
        } else if (arg == "--stats-interval") {
            config.stats_interval_sec = parse_u32(need_value("--stats-interval"), "--stats-interval");
        } else if (arg == "--dry-run") {
            config.dry_run = true;
        } else if (arg == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        } else {
            throw std::invalid_argument("unknown app option: " + arg);
        }
    }

    config.burst_size = std::max<uint16_t>(1, config.burst_size);
    config.queue_size = std::max<std::size_t>(1, config.queue_size);
    config.stats_interval_sec = std::max<uint32_t>(1, config.stats_interval_sec);
    return config;
}

void append_ipv4_udp_packet(std::vector<uint8_t>& storage, uint16_t dst_port, uint16_t payload_len)
{
    storage.resize(sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr) + payload_len);
    auto* eth = reinterpret_cast<rte_ether_hdr*>(storage.data());
    rte_ether_addr src{{0x02, 0x00, 0x00, 0x00, 0x00, 0x01}};
    rte_ether_addr dst{{0x02, 0x00, 0x00, 0x00, 0x00, 0x02}};
    eth->src_addr = src;
    eth->dst_addr = dst;
    eth->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    auto* ipv4 = reinterpret_cast<rte_ipv4_hdr*>(storage.data() + sizeof(rte_ether_hdr));
    ipv4->version_ihl = RTE_IPV4_VHL_DEF;
    ipv4->type_of_service = 0;
    ipv4->total_length = rte_cpu_to_be_16(sizeof(rte_ipv4_hdr) + sizeof(rte_udp_hdr) + payload_len);
    ipv4->packet_id = 0;
    ipv4->fragment_offset = 0;
    ipv4->time_to_live = 64;
    ipv4->next_proto_id = IPPROTO_UDP;
    ipv4->hdr_checksum = 0;
    ipv4->src_addr = rte_cpu_to_be_32(RTE_IPV4(10, 0, 0, 1));
    ipv4->dst_addr = rte_cpu_to_be_32(RTE_IPV4(10, 0, 0, 2));

    auto* udp = reinterpret_cast<rte_udp_hdr*>(storage.data() + sizeof(rte_ether_hdr) + sizeof(rte_ipv4_hdr));
    udp->src_port = rte_cpu_to_be_16(4000);
    udp->dst_port = rte_cpu_to_be_16(dst_port);
    udp->dgram_len = rte_cpu_to_be_16(sizeof(rte_udp_hdr) + payload_len);
    udp->dgram_cksum = 0;
}

SimulatedPacket make_simulated_packet(const std::string& label, uint16_t dst_port, uint16_t payload_len)
{
    SimulatedPacket packet{};
    append_ipv4_udp_packet(packet.storage, dst_port, payload_len);
    packet.label = label;
    packet.mbuf.buf_addr = packet.storage.data();
    packet.mbuf.data_off = 0;
    packet.mbuf.data_len = static_cast<uint16_t>(packet.storage.size());
    packet.mbuf.pkt_len = static_cast<uint32_t>(packet.storage.size());
    packet.mbuf.nb_segs = 1;
    packet.mbuf.next = nullptr;
    return packet;
}

std::vector<SimulatedPacket> build_simulated_traffic()
{
    std::vector<SimulatedPacket> packets;
    for (int i = 0; i < 6; ++i) {
        packets.push_back(make_simulated_packet("H" + std::to_string(i), 5001, 64));
    }
    for (int i = 0; i < 5; ++i) {
        packets.push_back(make_simulated_packet("M" + std::to_string(i), 5002, 512));
    }
    for (int i = 0; i < 4; ++i) {
        packets.push_back(make_simulated_packet("L" + std::to_string(i), 5003, static_cast<uint16_t>(256 + i * 256)));
    }
    return packets;
}

void run_dry_run_for(SchedulerType scheduler_type)
{
    AppConfig config{};
    config.scheduler = scheduler_type;
    config.dry_run = true;
    config.queue_size = 64;

    PacketParser parser;
    FlowClassifier classifier;
    auto scheduler = make_scheduler(config.scheduler);
    std::array<QosQueue, kPriorityCount> queues{QosQueue(config.queue_size), QosQueue(config.queue_size), QosQueue(config.queue_size)};
    auto packets = build_simulated_traffic();

    std::cout << "\n[dry-run] scheduler=" << scheduler->name() << '\n';
    for (auto& packet : packets) {
        auto parsed = parser.parse(&packet.mbuf);
        PriorityLevel level = classifier.classify(parsed);
        if (queues[priority_index(level)].enqueue(&packet.mbuf)) {
            std::cout << "  enqueue " << packet.label << " -> " << priority_name(level);
            if (parsed && parsed->flow.valid) {
                std::cout << " flow=" << parsed->flow.to_string();
            }
            std::cout << '\n';
        }
    }

    QueueViews views = make_queue_views(queues);
    std::cout << "  schedule order: ";
    bool first = true;
    while (any_queue_has_packets(views)) {
        PriorityLevel level = scheduler->select_queue(views);
        rte_mbuf* mbuf = queues[priority_index(level)].dequeue();
        if (mbuf == nullptr) {
            continue;
        }
        scheduler->on_packet_sent(level, mbuf);
        if (!first) {
            std::cout << " -> ";
        }
        std::cout << priority_name(level) << '(' << rte_pktmbuf_pkt_len(mbuf) << "B)";
        first = false;
    }
    std::cout << "\n";
}

void run_all_dry_runs()
{
    run_dry_run_for(SchedulerType::StrictPriority);
    run_dry_run_for(SchedulerType::WeightedRoundRobin);
    run_dry_run_for(SchedulerType::DeficitRoundRobin);
}

void enqueue_packet(std::array<QosQueue, kPriorityCount>& queues, Stats& stats, rte_mbuf* packet, PriorityLevel level)
{
    if (!queues[priority_index(level)].enqueue(packet)) {
        stats.on_drop();
        rte_pktmbuf_free(packet);
    }
}

void drain_one_packet(std::array<QosQueue, kPriorityCount>& queues, Scheduler& scheduler, DpdkPort& port, Stats& stats)
{
    QueueViews views = make_queue_views(queues);
    if (!any_queue_has_packets(views)) {
        return;
    }

    PriorityLevel level = scheduler.select_queue(views);
    rte_mbuf* packet = queues[priority_index(level)].dequeue();
    if (packet == nullptr) {
        return;
    }

    rte_mbuf* tx_packets[1] = {packet};
    const uint16_t sent = port.tx_burst(tx_packets, 1);
    if (sent == 1) {
        scheduler.on_packet_sent(level, packet);
        stats.on_tx(level, rte_pktmbuf_pkt_len(packet));
    } else {
        stats.on_drop();
        rte_pktmbuf_free(packet);
    }
}

int run_dpdk_app(const AppConfig& config)
{
    DpdkPort port(config);
    if (!port.init()) {
        return 1;
    }

    PacketParser parser;
    FlowClassifier classifier;
    auto scheduler = make_scheduler(config.scheduler);
    Stats stats(config.stats_interval_sec);
    std::array<QosQueue, kPriorityCount> queues{QosQueue(config.queue_size), QosQueue(config.queue_size), QosQueue(config.queue_size)};
    std::vector<rte_mbuf*> rx_packets(config.burst_size);

    std::cout << "Running DPDK forwarding loop with scheduler=" << scheduler->name()
              << " port=" << config.port_id
              << " burst=" << config.burst_size
              << " queue_size=" << config.queue_size << '\n';

    while (true) {
        const uint16_t nb_rx = port.rx_burst(rx_packets.data(), config.burst_size);
        for (uint16_t i = 0; i < nb_rx; ++i) {
            rte_mbuf* packet = rx_packets[i];
            stats.on_rx(rte_pktmbuf_pkt_len(packet));
            const auto parsed = parser.parse(packet);
            const PriorityLevel level = classifier.classify(parsed);
            enqueue_packet(queues, stats, packet, level);
        }

        for (uint16_t i = 0; i < config.burst_size; ++i) {
            drain_one_packet(queues, *scheduler, port, stats);
        }

        stats.maybe_print(queues, *scheduler);
    }
}

} // namespace

SchedulerType parse_scheduler_type(const std::string& value)
{
    if (value == "sp") {
        return SchedulerType::StrictPriority;
    }
    if (value == "wrr") {
        return SchedulerType::WeightedRoundRobin;
    }
    if (value == "drr") {
        return SchedulerType::DeficitRoundRobin;
    }
    throw std::invalid_argument("scheduler must be one of: sp, wrr, drr");
}

const char* scheduler_type_name(SchedulerType type)
{
    switch (type) {
    case SchedulerType::StrictPriority:
        return "sp";
    case SchedulerType::WeightedRoundRobin:
        return "wrr";
    case SchedulerType::DeficitRoundRobin:
        return "drr";
    }
    return "unknown";
}

std::string FlowKey::to_string() const
{
    auto ip_to_string = [](uint32_t ip) {
        return std::to_string((ip >> 24) & 0xff) + "." + std::to_string((ip >> 16) & 0xff) + "." +
               std::to_string((ip >> 8) & 0xff) + "." + std::to_string(ip & 0xff);
    };
    return ip_to_string(src_ip) + ":" + std::to_string(src_port) + " -> " + ip_to_string(dst_ip) + ":" +
           std::to_string(dst_port) + " proto=" + std::to_string(protocol);
}

const char* priority_name(PriorityLevel level)
{
    switch (level) {
    case PriorityLevel::High:
        return "HIGH";
    case PriorityLevel::Medium:
        return "MEDIUM";
    case PriorityLevel::Low:
        return "LOW";
    }
    return "UNKNOWN";
}

std::size_t priority_index(PriorityLevel level)
{
    return static_cast<std::size_t>(level);
}

void Scheduler::on_packet_sent(PriorityLevel, const rte_mbuf*)
{
}

} // namespace qos

int main(int argc, char** argv)
{
    const bool dry_run_requested = std::any_of(argv + 1, argv + argc, [](const char* arg) {
        return std::strcmp(arg, "--dry-run") == 0;
    });

    try {
        if (dry_run_requested) {
            qos::AppConfig config = qos::parse_app_args(argc, argv, 1);
            (void)config;
            qos::run_all_dry_runs();
            return 0;
        }

        int eal_argc = argc;
        int ret = rte_eal_init(argc, argv);
        if (ret < 0) {
            std::cerr << "rte_eal_init failed\n";
            return 1;
        }

        eal_argc -= ret;
        char** app_argv = argv + ret;
        qos::AppConfig config = qos::parse_app_args(eal_argc, app_argv);
        return qos::run_dpdk_app(config);
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << "\n\n";
        qos::print_usage(argv[0]);
        return 1;
    }
}
