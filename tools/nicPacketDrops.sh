#!/bin/bash

cmd="${0##*/}"

usage() {
cat >&2 <<EOI
usage: $cmd [ -h ]

Output column definitions:
    Device  ID

    total  # of packets recieved

    Errors # of errors on the NIC

    Dropped # of dropped packets

EOI
	exit 1
}

net_stats_header() {
    printf "%32s %33s\n" Receive Transmit
	printf "%20s %10s %10s %10s %10s %10s %10s\n" Interface total errors dropped total errors dropped
}

net_stats_format() {
	printf "%20s %10lu %10lu %10lu %10lu %10lu %10lu\n" "$1" "$2" "$3" "$4" "$5" "$6" "$7"
}

read_net() {
   net_stats_header
   while read Interface Rbytes Rtotal Rerrors Rdropped Rfifo Rframe Rcompressed Rmulticast Tbytes Ttotal Terrors Tdropped Tfifo Tframe Tcompressed Tmulticast; do

	   net_stats_format "$Interface" "$Rtotal" "$Rerrors" "$Rdropped" "$Ttotal" "$Terrors" "$Tdropped"
   done < <(tail -n +3 /proc/net/dev)
}
getopts h flag && usage
export -f read_net
export -f net_stats_format
export -f net_stats_header

watch -d -x bash -c "read_net"
