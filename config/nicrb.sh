#!/bin/bash
#
# Copyright 2021-2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
main(){
ETH_CARDS=($(ip link | grep -E -o '[0-9]+: [0-9­A-Z­a-z]+:' | sed 's/^[0-9]*: //;s/://'))
echo ""
echo "These are all of your links. Only modify the links that are connected to your USRPs"
echo "Please update this script with the correct link names and rx/tx ring buffer sizes"
echo "To get the maximum rx/tx ring buffer sizes run \"sudo ethtool -g {Your NIC}\""
echo "The Pre-set maximums are the maximum ring buffer sizes"
echo ""
print_arry "${ETH_CARDS[@]}"
echo ""
########################################################################
########################################################################

# Uncomment set_static when you you date the link_name variables and values!

########################################################################
########################################################################

#set_static

}

set_static(){
link_name=("ens10f4" "ens10f5" "ens10f6" "ens10f7" "ens10f0" "ens10f1" "ens10f2" "ens10f3" "ens4f4" "ens4f5" "ens4f6" "ens4f7" "ens4f0" "ens4f1" "ens4f2" "ens4f3")
for link in "${link_name[@]}"
do
    sudo ethtool -G $link tx 8160 rx 8160
done
}


#echo ${ETH_CARDS[4]}
#ethtool -g ${VAR[4]} print 1
#print 1
#rate 4096
#set 1,2,3,4-7
#set on boot

print_arry(){
    i=$({
        for elm in "$@";do
            echo "\"$elm\" "
        done | sort -g
    })
    echo "$i" | tr -d '\n' | tr -d '\r'
    return 0
}

#used to have main code at the top 
# '$@' is used to make the command line arguments of the script visible in the function

main "$@"; exit