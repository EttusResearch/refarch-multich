#!/bin/bash

cmd="${0##*/}"

usage() {
cat >&2 <<EOI
usage: $cmd [ -h ]

Output column definitions:
      cpu  # of the cpu 

    total  # of packets

  dropped  # of packets that were dropped because netdev_max_backlog was exceeded.
             If dropping packets increase the net.core.netdev_max_backlog.

 squeezed  # of times ksoftirq ran out of netdev_budget or time slice with work
             remaining. If packets are squeezing increase the net.core.netdev_budget_usecs
             and the net.core.netdev_budget

EOI
	exit 1
}

net_stats_header() {
	printf "%3s %10s %10s %10s %10s %11s\n" cpu total dropped squeezed rps flow_limit
}

net_stats_format() {
	printf "%3u %10lu %10lu %10lu %10lu %10lu\n" "0x$1" "0x$2" "0x$3" "0x$4" "0x$5" "0x$6"
}

read_net() {
   net_stats_header
   while read total dropped squeezed j1 j2 j3 j4 j5 collision rps flow_limit_count backlog index; do
	# Values Originate from Kernal
	# https://git.launchpad.net/~ubuntu-kernel/ubuntu/+source/linux/+git/focal/tree/net/core/net-procfs.c?h=hwe-5.11
	   net_stats_format "$index" "$total" "$dropped" "$squeezed" "$rps" "${flow_limit_count:-0}"
   done < /proc/net/softnet_stat
}
getopts h flag && usage
export -f read_net
export -f net_stats_format
export -f net_stats_header

watch -d -x bash -c "read_net"
