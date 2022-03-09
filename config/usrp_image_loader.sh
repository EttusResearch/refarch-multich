#!/bin/bash

#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

#Updates the FPGA Images on Each USRP

main(){
    check_root

    rm usrp_config.txt
    echo "Determining USRP IP Addresses"
    sudo uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    
    FILENAME="usrp_config.txt"
   
    #loop over ip addresses
    cat 'usrp_config.txt'
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))
    printf '%s\n' "${myarr[@]}"
    
    sudo uhd_images_downloader
    
    echo 'WARNING: This proceedure updates ALL USRPs connected to this host.'
    tempText="Would you like to update the FPGA Images of all USRPs?"
    if user_input "$tempText"; then
    
      for i in "${myarr[@]}"
      do

        echo "USRP: "$i
      
        update_image "$i" &
      
        
      done
      wait
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


update_image(){

  local i=$1
  uhd_image_loader --args "type=n3xx,addr=$i,fpga=XG"
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
