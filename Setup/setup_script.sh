###instructions:
###1) open a terminal and set working directory to tools directory
###2) execute the this script with root priveleges "sudo ./setup_script.sh"

#Set working directory to script location.
DIR="$(dirname "$(readlink -f "$0")")"
cd "$DIR"

#Adjust Network Buffers
echo '###Adjust Network Buffers###' >> /etc/sysctl.conf
echo 'net.core.wmem_max=33554432' >> /etc/sysctl.conf
echo 'net.core.rmem_max=33554432' >> /etc/sysctl.conf
echo 'net.core.wmem_default=33554432' >> /etc/sysctl.conf
echo 'net.core.rmem_default=33554432' >> /etc/sysctl.conf

#install dependencies
#Excluded argparse because I believe its installed in -dev
apt-get update
apt-get -y upgrade
apt-get -y install -q build-essential ccache git libffi-dev \
libssl-dev curl libboost-all-dev libusb-1.0-0-dev libudev-dev libgps-dev \
doxygen cmake libgps-dev python3-dev python3-pip python3-matplotlib \
python3-mako python3-docutils python3-requests python3-numpy python3-venv \
abi-dumper dpdk libdpdk-dev ncompress libncurses5-dev libncursesw5-dev



#This file will add UHD libraries to the enviroment so UHD commands
#can be executed without a setup step.
cp -f export_uhd_lib.sh /etc/profile.d/

#Download the UHD driver. 
#Only get the Tag release not all changes --depth 1
mkdir -p ../external
cd ../external
git clone --branch v4.1.0.1 --depth 1 https://github.com/EttusResearch/uhd.git

#Build the UHD driver and install it to system
mkdir uhd/host/build
cd uhd/host/build
cmake ..
make
make install

#Build cogrf host
cd "$DIR"
mkdir ../RefDesign/build
cd ../RefDesign/build
cmake ..
make

