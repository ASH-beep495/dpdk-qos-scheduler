#!/usr/bin/env bash
# SPDX-License-Identifier: MIT
set -euo pipefail

SCHEDULER="${1:-drr}"
PORT="${2:-0}"

sudo ./build/dpdk-qos-scheduler -l 0-1 -n 4 -- \
  --port "${PORT}" \
  --scheduler "${SCHEDULER}" \
  --burst-size 32 \
  --queue-size 1024 \
  --stats-interval 1
