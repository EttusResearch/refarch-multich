#!/bin/bash
#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

###1) open a terminal and set working directory to tools directory
###2) execute the this script with root priveleges "sudo ./setup_script.sh"

main(){

  #check for sudo Priveleges
  check_root

  #Set working directory to script location.
  DIR="$(dirname "$(readlink -f "$0")")"
  cd "$DIR"


  #install dependencies
  #Excluded argparse because I believe its installed in -dev
  apt-get update
  apt-get -y upgrade
  apt-get -y install -q build-essential ccache git libffi-dev \
  libssl-dev curl libboost-all-dev libusb-1.0-0-dev libudev-dev libgps-dev \
  doxygen cmake libgps-dev python3-dev python3-pip python3-matplotlib \
  python3-mako python3-docutils python3-requests python3-numpy python3-venv \
  abi-dumper libdpdk-dev ncompress libncurses5-dev libncursesw5-dev ethtool


  #Checks to see if UHD is already installed and will skip the installation.
  if find_UHD_install; then
    echo "Found installation of UHD skipping install!"
  else
    #This file will add UHD libraries to the enviroment so UHD commands
    #can be executed without a setup step.
    cp -f config/export_uhd_lib.sh /etc/profile.d/

    #Download the UHD driver. 
    #Only get the Tag release not all changes --depth 1
    mkdir -p uhd_GitSource
    cd uhd_GitSource
    git clone --branch UHD-4.0 https://github.com/EttusResearch/uhd.git
    cd uhd
    git checkout a7948beeef648fecd9646bbeaa63126d55cf1fc3


    #Build the UHD driver and install it to system
    mkdir host/build
    cd host/build
    cmake ..
    make -j
    make install

    #Update Library files to include UHD for the cogrf build
    ldconfig /usr/local/lib/
  fi

  ##########
  #Tuning https://kb.ettus.com/USRP_Host_Performance_Tuning_Tips_and_Tricks
  ##########
  #CPU_governor to set all to performance
  cpu_governor
  #Adds current user to a thread priority group called USRP
  thread_priority
  LOGOUT=$?
  #Adjust Network Buffers
  network_buffers_update
  sysctl -p /etc/sysctl.conf

  #Build cogrf
  cd "$DIR"
  mkdir build
  cd build
  cmake ..
  make
  cd "$DIR"
  chmod 777 -R build
  chmod 777 -R uhd_GitSource

  #Logout to set changes. Must be at end of script.
  if [[ $LOGOUT ]]; then
    echo ''
    echo 'In order for the ThreadPriority to take affect you must logout'
    tempText="Would you like to logout now? "
    if user_input "$tempText"; then
      pkill -KILL -u $(logname)
    fi
  fi
  exit 0 
}


############Functions###########

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

#finds UHD install and returns 1 if you need to install UHD

find_UHD_install(){
VAR=$(ldconfig -p | grep "libuhd.so.4")
if [[ $VAR == *"libuhd"* ]]; then
  if [[ $VAR == *"libuhd.so.4.0.0"* ]]; then
    return 0
  else
    echo ""
    echo "Warning: you dont have the correct version Looking for UHD4.1.0. Instead found"
    echo $VAR
    echo "A minimum version of 4.0.0 is required"
    echo ""
    tempText="Would you like to install UHD 4.0.0? "
    if user_input "$tempText"; then
      return 1
    else
       tempText="Would you like to compile without the supported version of UHD? "
      if user_input "$tempText"; then
        return 0
      else
        exit 0
      fi
    fi
  fi
else
  tempText="Would you like to install UHD 4.0.0?"
  if user_input "$tempText"; then
    return 1
  else
    return 0
  fi
fi
}

check_root(){
    if [ `whoami` == root ]; then
    echo "(1) already root"
  else
    echo "Error: Please run this as root with the following command"
    echo "sudo ./setup_script.sh"
    exit 1
  fi
}

#checks network buffers and updates the values if they arn't high enough
network_buffers_update(){
    VAR=$(cat /etc/sysctl.conf | grep "net.core")
    if [[ $VAR == *"net.core"* ]]; then
        VAR=$(cut -d "=" -f2- <<< "$VAR")
        if [[ $VAR != *"33554432"* ]]; then
            VAR=33554432
            sed -i -e "s/\(net.core.wmem_max=\).*/\1$VAR/" \
            -e "s/\(net.core.rmem_max=\).*/\1$VAR/" \
            -e "s/\(net.core.wmem_default=\).*/\1$VAR/" \
            -e "s/\(net.core.rmem_default=\).*/\1$VAR/" /etc/sysctl.conf
        fi
    else
        echo '###Adjust Network Buffers###' >> /etc/sysctl.conf
        echo 'net.core.wmem_max=33554432' >> /etc/sysctl.conf
        echo 'net.core.rmem_max=33554432' >> /etc/sysctl.conf
        echo 'net.core.wmem_default=33554432' >> /etc/sysctl.conf
        echo 'net.core.rmem_default=33554432' >> /etc/sysctl.conf
    fi
}

cpu_governor(){
  echo ''
  echo 'NI Recommends you set cores to performance for maximum streaming rates'
  tempText="Would you like to install cpufreq and set cores to performance? "
  if user_input "$tempText"; then
    apt-get -y install -q cpufrequtils
    for ((i=0;i<$(nproc);i++)) do 
      sudo cpufreq-set -c $i -r -g performance
    done
  fi
}

thread_priority(){
  echo ''
  echo 'NI Recommends you set thread priority for maximum streaming rates'
  tempText="Would you like to set thread priority by setting your current user in the usrp group? "
  if user_input "$tempText"; then
    groupadd usrp
    usermod -aG usrp $(logname)
    VAR=$(cat /etc/security/limits.conf | grep "@usrp")
    if [[ $VAR != *"@usrp - rtprio"* ]]; then
      echo '@usrp - rtprio  99' >> /etc/security/limits.conf
    fi
    return 0

  else
    return 1
  fi
}
#used to have main code at the top 
# '$@' is used to make the command line arguments of the script visible in the function
main "$@"; exit