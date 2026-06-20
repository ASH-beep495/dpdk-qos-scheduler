# SPDX-License-Identifier: MIT
project(
  'dpdk-qos-scheduler',
  'cpp',
  version: '0.1.0',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    'buildtype=debugoptimized'
  ]
)

dpdk_dep = dependency('libdpdk', method: 'pkg-config', required: true)

inc = include_directories('include')

sources = files(
  'src/main.cpp',
  'src/packet_parser.cpp',
  'src/flow_classifier.cpp',
  'src/qos_queue.cpp',
  'src/sp_scheduler.cpp',
  'src/wrr_scheduler.cpp',
  'src/drr_scheduler.cpp',
  'src/stats.cpp',
  'src/dpdk_port.cpp'
)

executable(
  'dpdk-qos-scheduler',
  sources,
  include_directories: inc,
  dependencies: dpdk_dep,
  install: false
)
