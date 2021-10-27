#!/bin/bash

#

main(){
    check_root

    echo "Determining USRP IP Addresses"
    sudo uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    
    FILENAME="usrp_config.txt"
   
    #loop over ip addresses
    cat 'usrp_config.txt'
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))
    printf '%s\n' "${myarr[@]}"
    
    sudo uhd_images_downloader
    
    for i in "${myarr[@]}"
    do

      echo "USRP: "$i
     
      update_image "$i" &
     
      
    done
    wait  

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


update_image(){

  local i=$1
  uhd_image_loader --args "type=n3xx,addr=$i,fpga=XG"
}


main "$@"; exit