#!/bin/bash

#This script updates the ip addresses of each USRP SFP1 Port. 
#Change sfp1 to sfp0 for the sfp0 port. 

main(){
    check_root
    rm usrp_config.txt
    echo "Determining USRP IP Addresses"
    uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    FILENAME="usrp_config.txt"
    #loop over ip addresses
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))
    rm USRP_IP.txt
    rm sfp1.network
    echo 'WARNING: This proceedure updates ALL USRPs connected to this host.'
    tempText="Would you like to update the SFP1 IP address of all USRPs?"
    if user_input "$tempText"; then
      COUNTER=100
      for i in "${myarr[@]}"
      do
        echo "USRP: "$i
        #echo "ENTER SFP ADDRESS"
        #read sfp_address
        sfp_address=192.168.$COUNTER.2
        echo "SFP1 ADDRESS: " $sfp_address

        printf "[Match]\n" > sfp1.network
        printf "Name=sfp1\n" >>sfp1.network
        printf "\n" >> sfp1.network
        printf "[Network]\n" >> sfp1.network
        printf "Address=$sfp_address/24\n" >> sfp1.network
        printf "\n" >> sfp1.network
        printf "[Link]\n" >> sfp1.network
        printf "MTUBytes=9000" >> sfp1.network
        

        printf "Mgmnt IP: $i\n" >> USRP_IP.txt
        printf "SFP1 IP: $sfp_address\n" >> USRP_IP.txt
        
        ssh root@$i 'rm /data/network/sfp1.network'
        scp sfp1.network root@$i:/data/network 
        let COUNTER++
      done 
    fi
    exit 0
}

check_root(){
    if [ `whoami` == 'root' ]; then
    echo "(1) already root"
  else
    echo "Error: Please run this as root with the following command"
    echo "sudo ./usrp_config.sh"
    exit 1
  fi
}

#requests User input function
user_input(){
  echo ""
  echo "Please answer yes or no: "
  while true; do
      read -p "$1" yn
      case $yn in
          [Yy]* ) return 0;;
          [Nn]* ) return 1;;
          * ) echo "Please answer yes or no: ";;
      esac
  done
}

main "$@"; exit
