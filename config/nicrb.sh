#!/bin/bash
ethtool_setup(){
    link_name=("ens4f0" "ens4f1" "ens4f2" "ens4f3" "ens6f0" "ens6f1" "ens6f2" "ens6f3" "ens9f0" "ens9f1" "ens9f2" "ens9f3" "ens10f0" "ens10f1" "ens10f2" "ens10f3")
    for link in "${link_name[@]}"
    do
        sudo ethtool -G $link tx 4096 rx 4096
    done
}
main(){
ETH_CARDS=($(ip link | grep -E -o '[0-9]+: [0-9­A-Z­a-z]+:' | sed 's/^[0-9]*: //;s/://'))

print_arry "${ETH_CARDS[@]}"
ethtool_setup

}

#echo ${ETH_CARDS[4]}
#ethtool -g ${VAR[4]}

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