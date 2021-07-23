###instructions:
###0) leave the cloned repo hierarchy as is (don't move any files)
###1) open a terminal and set working directory to tools directory
###2) configure permission to allow execution of script "sudo chmod +x setup_script.sh"
###3) execute the the setup script "sudo ./setup_script.sh"

#Set working directory to script location.
DIR="$(dirname "$(readlink -f "$0")")"
cd "$DIR"
mkdir -p ../external

#Adjust Network Buffers
echo '###Adjust Network Buffers###' >> /etc/sysctl.conf
echo 'net.core.wmem_max=33554432' >> /etc/sysctl.conf
echo 'net.core.rmem_max=33554432' >> /etc/sysctl.conf
echo 'net.core.wmem_default=33554432' >> /etc/sysctl.conf
echo 'net.core.rmem_default=33554432' >> /etc/sysctl.conf

#Persistently set CPUs to 'performance' by removing ondemand service
#https://askubuntu.com/questions/929884/how-to-set-performance-instead-of-powersave-as-default
####TODO: this doesn't persist through reboots
#sudo apt-get install cpufrequtils
#echo 'GOVERNOR="performance"' | sudo tee /etc/default/cpufrequtils
#sudo systemctl disable ondemand


#install dependencies
apt-get update
apt-get -y upgrade
apt-get -y install -q build-essential ccache git libffi-dev mdadm \
libssl-dev curl libboost-all-dev libusb-1.0-0-dev libudev-dev libgps-dev \
doxygen cmake libgps-dev python3-dev python3-pip python3-mako \
python3-docutils python3-requests python3-numpy python3-venv \
python3-tk abi-dumper ncompress libncurses5-dev libncursesw5-dev
#symbolic link enables execution of python scripts from command line
ln -s /usr/bin/python3 /usr/bin/python

#Download the DPDK driver. https://doc.dpdk.org/guides-18.11/linux_gsg/build_dpdk.html
cd "$DIR"
cd ../external
curl https://fast.dpdk.org/rel/dpdk-18.11.11.tar.xz --output dpdk-18.11.11.tar.xz
tar xJf dpdk-18.11.11.tar.xz

#Build the DPDK driver and install it to system  https://files.ettus.com/manual/page_dpdk.html
cd dpdk-stable-18.11.11
make config T=x86_64-native-linuxapp-gcc
cd build
sed -i "s:CONFIG_RTE_BUILD_SHARED_LIB=n:CONFIG_RTE_BUILD_SHARED_LIB=y:" .config
make 
make install

#create symbolic links to dpdk libraries in location that the UHD driver can load
mkdir -p /usr/local/lib/dpdk-pmds 
for i in /usr/local/lib/librte_mempool_*
do
ln -s "$i" /usr/local/lib/dpdk-pmds/"${i##*/}"
done
for i in /usr/local/lib/librte_pmd_*
do
ln -s "$i" /usr/local/lib/dpdk-pmds/"${i##*/}"
done

#dpdk 'make install' doesn't install devbind tool so do it manually
cd ../usertools
cp ./dpdk-devbind.py /usr/bin

#This file will add UHD libraries to the enviroment so UHD commands
#can be executed without a setup step.
#TODO consider using /etc/ld.so.conf.d/ instead
#   echo "/usr/local/lib" >> /etc/ld.so.conf.d/uhd_lib.conf
#   ldconfig
cp -f export_uhd_lib.sh /etc/profile.d/

#Download the UHD driver.
cd "$DIR"
cd ../external
git clone --single-branch --branch UHD-4.0 https://github.com/EttusResearch/uhd.git

#Build the UHD driver and install it to system
mkdir -p uhd/host/build
cd uhd/host/build
cmake -DENABLE_DPDK=ON ..
make
make install

#Build cogrf host
cd "$DIR"
mkdir -p ../host/build
cd ../host/build
cmake ..
make

#Build python venv in host/build
python3 -m venv ./examples/python/venv
./examples/python/venv/bin/pip install numpy matplotlib pytest
