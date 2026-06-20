#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

PCI_ADDR="${1:-}"
DRIVER="${2:-vfio-pci}"

if [[ -z "${PCI_ADDR}" ]]; then
  echo "Usage: sudo $0 <pci-addr> [driver]" >&2
  echo "Example: sudo $0 0000:01:00.0 vfio-pci" >&2
  exit 1
fi

if [[ "${EUID}" -ne 0 ]]; then
  echo "Please run as root." >&2
  exit 1
fi

if [[ -z "${RTE_SDK:-}" ]]; then
  echo "RTE_SDK is not set. Trying dpdk-devbind.py from PATH..."
  dpdk-devbind.py --bind="${DRIVER}" "${PCI_ADDR}"
else
  "${RTE_SDK}/usertools/dpdk-devbind.py" --bind="${DRIVER}" "${PCI_ADDR}"
fi

echo "Bound ${PCI_ADDR} to ${DRIVER}"
