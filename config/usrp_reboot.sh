#!/bin/bash

main(){
    check_root

    echo "Determining USRP IP Addresses"
    #uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    
    FILENAME="usrp_config.txt"
   

    #while read -r line; do
    #  if [[ $line =~ ^[A-Za-z0-9:]+[" "]+[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
    #    ip=$line
    #   echo "$ip" 
    # fi
    #done < "$FILENAME"

    #loop over ip addresses
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))

    COUNTER=100
    for i in "${myarr[@]}"
    do

      echo "Rebooting USRP: "$i
      
      
      ssh root@$i 'reboot'
      

      #rm sfp1.network
      let COUNTER++
    done 

    exit 0
}

check_root(){
    if [ `whoami` == root ]; then
    echo "(1) already root"
  else
    echo "Error: Please run this as root with the following command"
    echo "sudo ./usrp_config.sh"
    exit 1
  fi
}

user_input(){
  echo ""
  echo "ENTER USRP SFP ADDRESS: "
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