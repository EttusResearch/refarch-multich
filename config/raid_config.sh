#!/bin/bash


#Helper Script for creating RAID arrays

main(){
check_root

echo 'WARNING: This proceedure creates RAID arrays. Only proceed if you have an understanding of your system and what is being done.'
    tempText="Would you like to proceed?"
    if user_input "$tempText"; then
      ### CREATE RAID
      #make the RAID0 array
      sudo mdadm --create --verbose /dev/md0 --level=0 --raid-devices=4 /dev/nvme0n1 /dev/nvme1n1 /dev/nvme2n1 /dev/nvme3n1

      #create and mount a file system
      sudo mkfs.ext4 -F /dev/md0
      sudo mkdir -p /mnt/md0
      sudo mount /dev/md0 /mnt/md0

      #save the array configuration
      sudo mdadm --detail --scan | sudo tee -a /etc/mdadm/mdadm.conf
      sudo update-initramfs -u
      echo '/dev/md0 /mnt/md0 ext4 defaults,nofail,discard 0 0' | sudo tee -a /etc/fstab

      #set access permission
      sudo chown $USER:$USER /mnt/md0

      ### CREATE RAID
      #make the RAID1 array
      sudo mdadm --create --verbose /dev/md1 --level=0 --raid-devices=4 /dev/nvme4n1 /dev/nvme5n1 /dev/nvme6n1 /dev/nvme7n1

      #create and mount a file system
      sudo mkfs.ext4 -F /dev/md1
      sudo mkdir -p /mnt/md1
      sudo mount /dev/md1 /mnt/md1

      #save the array configuration
      sudo mdadm --detail --scan | sudo tee -a /etc/mdadm/mdadm.conf
      sudo update-initramfs -u
      echo '/dev/md1 /mnt/md1 ext4 defaults,nofail,discard 0 0' | sudo tee -a /etc/fstab

      #set access permission
      sudo chown $USER:$USER /mnt/md1
    fi 

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