###instructions:
###1) open a terminal and set working directory to tools directory
###2) execute the this script with root priveleges "sudo ./setup_script.sh"

#Set working directory to script location.
cd "${0%/*}"

#install dependencies
apt-get update
apt-get -y upgrade
apt-get -y install -q build-essential ccache git libffi-dev \
libssl-dev curl libboost-all-dev libusb-1.0-0-dev libudev-dev libgps-dev \
doxygen cmake libgps-dev python3-dev python3-pip python3-mako \
python3-docutils python3-requests python3-numpy python3-venv \
abi-dumper dpdk libdpdk-dev ncompress libncurses5-dev libncursesw5-dev


#This file will add UHD libraries to the enviroment so UHD commands
#can be executed without a setup step.
cp -f export_uhd_lib.sh /etc/profile.d/

#Download the UHD driver.
mkdir -p ../external
cd ../external
git clone --single-branch --branch UHD-4.0 https://github.com/EttusResearch/uhd.git

#Build the UHD driver and install it to system
mkdir uhd/host/build
cd uhd/host/build
cmake ..
make
make install

#Build cogrf host
cd "${0%/*}"
mkdir ../host/build
cd ../host/build
cmake ..
make

#Build python venv
python3 -m venv ./examples/python/venv
./examples/python/venv/bin/pip install numpy matplotlib argparse
