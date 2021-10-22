#!/bin/bash

main(){
    check_root

    echo "Determining USRP IP Addresses"
    sudo uhd_find_devices > usrp_config.txt
    echo 'Information Stored in usrp_config.txt'
    
    FILENAME="usrp_config.txt"
   

    #while read -r line; do
    #  if [[ $line =~ ^[A-Za-z0-9:]+[" "]+[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}$ ]]; then
    #    ip=$line
    #   echo "$ip" 
    # fi
    #done < "$FILENAME"

    #loop over ip addresses
    cat 'usrp_config.txt'
    myarr=($(awk '$1 ~ /addr:/ && $1 !~ /mgmt_addr/ {print $2}' usrp_config.txt))
    printf '%s\n' "${myarr[@]}"
    
    for i in "${myarr[@]}"
    do

      echo "USRP: "$i
      #echo "ENTER SFP ADDRESS"
      #read sfp_address
      update "$i" &
      #ssh root@$i 'mender -commit'
      #uhd_image_loader --args "type=n3xx,addr=$i,fpga=XG"
      
      
      
      #rm sfp1.network
      
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

update(){

  local i=$1
  #scp /usr/local/share/uhd/images/usrp_n3xx_fs.mender root@$i:~/.
  #ssh root@$i 'mender install /home/root/usrp_n3xx_fs.mender'
  #ssh root@$i 'reboot' 
  #ssh root@$i 'mender -commit'
  uhd_image_loader --args "type=n3xx,addr=$i,fpga=XG"
}


main "$@"; exit