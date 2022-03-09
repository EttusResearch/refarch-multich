#!/bin/bash
#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
main(){

    #check for sudo Priveleges
    check_root

    tempText="Would you like to remove UHD? "

    if user_input "$tempText"; then
    echo "Removing UHD"
        #Set working directory to script location.
        DIR="$(dirname "$(readlink -f "$0")")"
        cd "$DIR"
        cd uhd_GitSource/uhd/host/build
        make uninstall -j
        cd "$DIR"
        rm -r uhd_GitSource
        cd /usr/local/lib
        rm -r uhd
        cd /usr/local/share
        rm -r uhd
        cd /usr/local/include
        rm -r uhd
        #Update Library files
        ldconfig /usr/local/lib/
        return 1
    else
    echo "Exiting"
        exit 0
    fi
    

}

check_root(){
    if [ `whoami` == root ]; then
    echo "(1) already root"
  else
    echo "Error: Please run this as root with the following command"
    echo "sudo ./remove_uhd.sh"
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

#used to have main code at the top 
# '$@' is used to make the command line arguments of the script visible in the function
main "$@"; exit