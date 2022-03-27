#!/usr/bin/env python3

import subprocess

import os
import sys

from util import *


""" def run_cpp_example(build_path: str, cfgFile: str):
    os.chdir("/home/ts-cogrf/Documents/dylan_workarea_dev/refarch-multich-dev/build")
    p = subprocess.Popen(["./"+build_path,"--cfgFile="+cfgFile], shell=False)
    out, err = p.communicate()
    errcode = p.returncode
    print(errcode)
    p.kill()
    p.terminate() """


def main():
    
    #folder_list = get_last_num_folders("/mnt/md0/", 10)
    ##    print(folder)

    
   run_cpp_example("/home/ts-cogrf/Documents/dylan_workarea_dev/refarch-multich-dev/build", "Arch_rfnoc_txrx_loopback", '/home/ts-cogrf/Documents/runconfig2.cfg')

if __name__ == "__main__":
    sys.exit(not main())