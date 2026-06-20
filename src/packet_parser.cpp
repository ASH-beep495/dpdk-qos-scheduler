// SPDX-License-Identifier: MIT
#include "packet_parser.hpp"

#include <arpa/inet.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <rte_udp.h>

namespace qos {

std::optional<ParsedPacket> PacketParser::parse(const rte_mbuf* mbuf) const
{
    if (mbuf == nullptr || rte_pktmbuf_pkt_len(mbuf) < sizeof(rte_ether_hdr)) {
        return std::nullopt;
    }

    const auto* eth = rte_pktmbuf_mtod(mbuf, const rte_ether_hdr*);
    uint16_t ether_type = rte_be_to_cpu_16(eth->ether_type);
    std::size_t offset = sizeof(rte_ether_hdr);

    if (ether_type == RTE_ETHER_TYPE_VLAN) {
        if (rte_pktmbuf_pkt_len(mbuf) < offset + sizeof(rte_vlan_hdr)) {
            return std::nullopt;
        }
        const auto* vlan = reinterpret_cast<const rte_vlan_hdr*>(reinterpret_cast<const uint8_t*>(eth) + offset);
        ether_type = rte_be_to_cpu_16(vlan->eth_proto);
        offset += sizeof(rte_vlan_hdr);
    }

    ParsedPacket parsed{};
    if (ether_type != RTE_ETHER_TYPE_IPV4) {
        return parsed;
    }

    if (rte_pktmbuf_pkt_len(mbuf) < offset + sizeof(rte_ipv4_hdr)) {
        return std::nullopt;
    }

    const auto* ipv4 = reinterpret_cast<const rte_ipv4_hdr*>(reinterpret_cast<const uint8_t*>(eth) + offset);
    const uint8_t ihl_bytes = static_cast<uint8_t>((ipv4->version_ihl & 0x0f) * 4);
    if (ihl_bytes < sizeof(rte_ipv4_hdr) || rte_pktmbuf_pkt_len(mbuf) < offset + ihl_bytes) {
        return std::nullopt;
    }

    parsed.is_ipv4 = true;
    parsed.flow.src_ip = rte_be_to_cpu_32(ipv4->src_addr);
    parsed.flow.dst_ip = rte_be_to_cpu_32(ipv4->dst_addr);
    parsed.flow.protocol = ipv4->next_proto_id;

    offset += ihl_bytes;
    if (ipv4->next_proto_id == IPPROTO_TCP) {
        if (rte_pktmbuf_pkt_len(mbuf) < offset + sizeof(rte_tcp_hdr)) {
            return std::nullopt;
        }
        const auto* tcp = reinterpret_cast<const rte_tcp_hdr*>(reinterpret_cast<const uint8_t*>(eth) + offset);
        parsed.is_tcp = true;
        parsed.flow.src_port = rte_be_to_cpu_16(tcp->src_port);
        parsed.flow.dst_port = rte_be_to_cpu_16(tcp->dst_port);
        parsed.flow.valid = true;
        return parsed;
    }

    if (ipv4->next_proto_id == IPPROTO_UDP) {
        if (rte_pktmbuf_pkt_len(mbuf) < offset + sizeof(rte_udp_hdr)) {
            return std::nullopt;
        }
        const auto* udp = reinterpret_cast<const rte_udp_hdr*>(reinterpret_cast<const uint8_t*>(eth) + offset);
        parsed.is_udp = true;
        parsed.flow.src_port = rte_be_to_cpu_16(udp->src_port);
        parsed.flow.dst_port = rte_be_to_cpu_16(udp->dst_port);
        parsed.flow.valid = true;
        return parsed;
    }

    parsed.flow.valid = true;
    return parsed;
}

} // namespace qos
