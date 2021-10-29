#!/bin/bash

#Reboot All USRPS
main(){
    check_root
    rm usrp_config.txt
    echo "Determining USRP IP Addresses"
    uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    FILENAME="usrp_config.txt"
    #loop over ip addresses
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))

    COUNTER=100
    echo 'WARNING: This proceedure reboots ALL USRPs connected to this host.'
    tempText="Would you like to reboot all USRPs?"
    if user_input "$tempText"; then
      for i in "${myarr[@]}"
      do
        echo "Rebooting USRP: "$i 
        ssh root@$i 'reboot'
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
