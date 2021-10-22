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
      #uhd_image_loader --args "type=n3xx,addr=$i,fpga=XG"
      
      #scp sfp1.network root@$i:/data/network 
      #ssh root@$i 'exit'
      #ssh root@$i 'reboot'
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