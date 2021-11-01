#!/bin/bash
#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

set_static(){
link_name=("ens10f4" "ens10f5" "ens10f6" "ens10f7" "ens10f0" "ens10f1" "ens10f2" "ens10f3" "ens4f4" "ens4f5" "ens4f6" "ens4f7" "ens4f0" "ens4f1" "ens4f2" "ens4f3")
for link in "${link_name[@]}"
do
    sudo ethtool -G $link tx 8160 rx 8160
done
}
main(){
ETH_CARDS=($(ip link | grep -E -o '[0-9]+: [0-9­A-Z­a-z]+:' | sed 's/^[0-9]*: //;s/://'))

print_arry "${ETH_CARDS[@]}"
set_static

}

#echo ${ETH_CARDS[4]}
#ethtool -g ${VAR[4]} print 1
#print 1
#rate 4096
#set 1,2,3,4-7
#set on boot

print_arry(){
    i=0
    for elm in $@ 
    do
        echo $i: $elm
        ((i+=1))
    done
    return 0
}

#used to have main code at the top 
# '$@' is used to make the command line arguments of the script visible in the function

main "$@"; exit