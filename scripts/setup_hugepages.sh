#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

PAGES="${1:-1024}"

if [[ "${EUID}" -ne 0 ]]; then
  echo "Please run as root: sudo $0 ${PAGES}" >&2
  exit 1
fi

echo "Allocating ${PAGES} hugepages..."
echo "${PAGES}" > /proc/sys/vm/nr_hugepages
mkdir -p /mnt/huge
if ! mountpoint -q /mnt/huge; then
  mount -t hugetlbfs nodev /mnt/huge
fi

grep Huge /proc/meminfo
echo "HugePages ready at /mnt/huge"
