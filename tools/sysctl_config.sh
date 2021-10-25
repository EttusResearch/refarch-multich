#!/bin/bash
#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

check_root(){
    if [ $EUID == 0 ]; then
    echo "(1) already root"
  else
    echo "Error: Please run this as root with the following command"
    echo "sudo ./$(basename "${BASH_SOURCE[0]}")"
    exit 1
  fi
}
update_netcore(){
    NET_name=$1
    NET_val=$2
    VAR=$(grep "$NET_name=" < /etc/sysctl.conf)
    if [[ $VAR == *"$NET_name"* ]]; then
        VAR=$(cut -d "=" -f2- <<< "$VAR")
        if [[ $VAR != *"$NET_val"* ]]; then
            sed -i -e "s/\($NET_name=\).*/\1$NET_val/" /etc/sysctl.conf
            echo "Set $NET_name = $NET_val"
        else
            echo "Nothing to do: $NET_name = $VAR"
        fi
    else
        echo "$NET_name=$NET_val" >> /etc/sysctl.conf
        echo "Created and Set $NET_name = $NET_val"
    fi
    sysctl -p > /dev/null
}

check_root

# Maximum and Default TX Buffer. increasing this too much will have adverse effects
update_netcore net.core.wmem_max            67108864
update_netcore net.core.wmem_default        67108864

# Maximum and Default RX Buffer. increasing this too much will have adverse effects
update_netcore net.core.rmem_max            67108864
update_netcore net.core.rmem_default        67108864

# netdev_max_backlog - is the maximum number of packets, queued on the INPUT side, 
#                      when the interface receives packets faster than kernel can process them.
update_netcore net.core.netdev_max_backlog  2000

# netdev_budget - is the maximum number of packets taken from all interfaces in one polling cycle. 
#                 In one polling cycle interfaces which are registered to polling are probed
update_netcore net.core.netdev_budget 300

# netdev_budget_usecs - maximum number of microseconds in one NAPI polling cycle. Polling will exit 
#                       when either netdev_budget_usecs have elapsed during the poll cycle or the 
#                       number of packets processed reaches netdev_budget
update_netcore net.core.netdev_budget_usecs 4500

# net.core.dev_weight - is the maximum number of packets that kernel can handle on a NAPI interrupt,
#                       it's a Per-CPU variable
update_netcore net.core.dev_weight 300

