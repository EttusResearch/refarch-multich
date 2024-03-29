#!/bin/bash
#
# Copyright 2021-2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
#Script for updating linux filesystem of ALL usrps connected to the host. 

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

    echo 'WARNING: This procedure updates ALL USRPs connected to this host.'
    tempText="Would you like to update the filesystem of all USRPs?"
    if user_input "$tempText"; then
      
      for i in "${myarr[@]}"
      do
        echo "USRP: "$i
        update_filesystem "$i" &
      done
      wait
      echo 'Rebooting USRPs..'
      for i in "${myarr[@]}"
      do
        reboot "$i" &
      done
      wait

      echo "Waiting 3 Minutes for reboot, do not interrupt."
      sleep 3m

      for i in "${myarr[@]}"
      do
        echo 'Running uhd_usrp_probe...'
        uhd_usrp_probe --args addr=$i
        echo 'Does the USRP at '$i ' appear to be functioning?'
        echo 'It is recommended that uhd_usrp_probe be used to verify functionality.'
        tempText="If the USRP appears to be functioning, would you like to commit the file system update?"
        if user_input "$tempText"; then
          echo -e "Committing file system...\n"
          mender_commit "$i" &
        else 
        echo -e "If the USRP does not appear to be functioning correctly, please reboot and try the update process manually. See the N320/N321 getting started guide for more details./n"
        fi
      done
      wait
    fi 
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
update_filesystem(){
  local i=$1
  scp /usr/local/share/uhd/images/usrp_n3xx_fs.mender root@$i:~/.
  ssh root@$i 'mender install /home/root/usrp_n3xx_fs.mender'
}
reboot(){
  local i=$1
  ssh root@$i 'reboot' 
}
mender_commit(){

  local i=$1
  
  ssh root@$i 'mender -commit'
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
