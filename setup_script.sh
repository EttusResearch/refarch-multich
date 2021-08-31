#!/bin/bash
###instructions:
###1) open a terminal and set working directory to tools directory
###2) execute the this script with root priveleges "sudo ./setup_script.sh"

main(){

  #check for sudo Priveleges
  check_root

  #Set working directory to script location.
  DIR="$(dirname "$(readlink -f "$0")")"
  cd "$DIR"

  #Adjust Network Buffers
  echo '###Adjust Network Buffers###' >> /etc/sysctl.conf
  echo 'net.core.wmem_max=33554432' >> /etc/sysctl.conf
  echo 'net.core.rmem_max=33554432' >> /etc/sysctl.conf
  echo 'net.core.wmem_default=33554432' >> /etc/sysctl.conf
  echo 'net.core.rmem_default=33554432' >> /etc/sysctl.conf

  sysctl -p /etc/sysctl.conf

  #install dependencies
  #Excluded argparse because I believe its installed in -dev
  apt-get update
  apt-get -y upgrade
  apt-get -y install -q build-essential ccache git libffi-dev \
  libssl-dev curl libboost-all-dev libusb-1.0-0-dev libudev-dev libgps-dev \
  doxygen cmake libgps-dev python3-dev python3-pip python3-matplotlib \
  python3-mako python3-docutils python3-requests python3-numpy python3-venv \
  abi-dumper dpdk libdpdk-dev ncompress libncurses5-dev libncursesw5-dev


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
    git clone --branch v4.1.0.1 --depth 1 https://github.com/EttusResearch/uhd.git

    #Build the UHD driver and install it to system
    mkdir uhd/host/build
    cd uhd/host/build
    cmake ..
    make
    make install

    #Update Library files to include UHD for the cogrf build
    ldconfig /usr/local/lib/
  fi

  #Build cogrf
  cd "$DIR"
  mkdir build
  cd build
  cmake ..
  make
  cd "$DIR"
  chmod 777 -R build
  chmod 777 -R uhd_GitSource
  exit 0 
}


############Functions###########

#requests User input function
user_input(){
while true; do
    read -p "$1" yn
    case $yn in
        [Yy]* ) return 0;;
        [Nn]* ) return 1;;
        * ) echo "Please answer yes or no.";;
    esac
done
}

#finds UHD install and returns 1 if you need to install UHD

find_UHD_install(){
VAR=$(ldconfig -p | grep "libuhd.so.4")
if [[ $VAR == *"libuhd"* ]]; then
  if [[ $VAR == *"libuhd.so.4.1.0"* ]]; then
    return 0
  else
    echo "Warning: you dont have the correct version Looking for UHD4.1.0. Instead found"
    echo $VAR
    echo "A minimum version of 4.0.0 is required"
    echo
    tempText="Would you like to install UHD 4.1.0?"
    if user_input "$tempText"; then
      return 1
    else
       tempText="Would you like to compile without the supported version of UHD?"
      if user_input "$tempText"; then
        return 0
      else
        exit 0;
      fi
    fi
  fi
else
  tempText="Would you like to install UHD 4.1.0?"
  if user_input "$tempText"; then
    return 1
  else
    exit 0;
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
#used to have main code at the top 
# '$@' is used to make the command line arguments of the script visible in the function
main "$@"; exit