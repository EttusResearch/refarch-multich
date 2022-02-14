#!/bin/bash
#
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

sudo mdadm --create --verbose /dev/md0 --level=0 --raid-devices=4 /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1
sudo mdadm --create --verbose /dev/md1 --level=0 --raid-devices=4 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1

sudo mkfs.ext4 -F /dev/md0 
sudo mkdir -p /mnt/md0 
sudo mount /dev/md0 /mnt/md0
sudo mkfs.ext4 -F /dev/md1 
sudo mkdir -p /mnt/md1 
sudo mount /dev/md1 /mnt/md1

sudo mdadm --detail --scan | sudo tee -a /etc/mdadm/mdadm.conf 
sudo update-initramfs -u 
echo '/dev/md0 /mnt/md0 ext4 defaults,nofail,discard 0 0' | sudo tee -a /etc/fstab
sudo mdadm --detail --scan | sudo tee -a /etc/mdadm/mdadm.conf 
sudo update-initramfs -u 
echo '/dev/md1 /mnt/md1 ext4 defaults,nofail,discard 0 0' | sudo tee -a /etc/fstab

sudo chown root:ts-cogrf /mnt/md0
sudo chown root:ts-cogrf /mnt/md1
sudo chmod g+rw /mnt/md0
sudo chmod g+rw /mnt/md1
