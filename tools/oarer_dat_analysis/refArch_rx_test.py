#!/usr/bin/env python3

#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

#test with PyTest

from genericpath import isdir
from util import *
import run_arch
import sys
import time
import datetime
import unittest
import os
import shutil
import logging as log


#@pytest.fixture
def move_data(params, iterations, temp_location, d1):
    # Move data to temp folder for analysis
    # Will move iterations number of folders
    
    folder_list0 = get_last_num_folders(params.output_path0, iterations)
    dst_list=[]
    if params.output_path1:
        folder_list1 = get_last_num_folders(params.output_path1, iterations)
    for folder in folder_list0:
        log.info(folder)
        src0 = folder
        shortpath= os.path.relpath(src0,params.output_path0)
        dst0 = os.path.join(temp_location,shortpath)
        dst_list.append(dst0)
        try:
            os.mkdir(dst0)
        except:
            print("Directory Exists")
        for file in os.listdir(src0):
            os.chdir(src0)
            shutil.copy2(file,os.path.join(dst0,""))        
    if params.output_path1:
        for folder1 in folder_list1:
            log.info(folder1)
            src1 = folder1
            os.chdir(src1)
            dst1 = os.path.join(temp_location,os.path.basename(folder1),"")
            for file in os.listdir(src1):
                shutil.copy2(file, dst1)
    print("Received Files moved to: {} ".format(temp_location))
    return temp_location


def test_ten_run(params, arch_params):
    #Run provided example 10 times
    ITERATIONS = 10
    now = datetime.datetime.now()
    d1 = "rx_test_" + now.strftime("%m_%d_%Y_%H_%M_%S")

    print("Running Arch Example {} times with the following arguments: ".format(ITERATIONS))
    print("cfgFile: " + params.cfgFile)
    for key, val in arch_params.items():
        print("{:14} {}".format(key, val))
    temp_location = os.getcwd() + "/temp/" + d1
    os.mkdir(temp_location)
    #Run example 10 times
    run_arch.run_batch(params.build_path, params.example, ITERATIONS, params.cfgFile, arch_params)
    #Move data to temp directory for analysis
    data_location = move_data(params,ITERATIONS,temp_location, d1)

    # Read in IQ Data 
    data0_folderdict = batch_folder_import(data_location, ITERATIONS, start_point = params.start_point, fs=params.fs, ppw=params.ppw)

    batch_analyze_plot(data0_folderdict, params.ppw, ITERATIONS, params.base_rx)
    


if __name__ == "__main__":
    arch_params, rest = run_arch.parse_known_args()
    parser = configargparse.ArgumentParser()
    parser.add_argument("--cfgFile", type=str, required=True, help="path to config file")
    parser.add_argument("--example", type=str, required=True, help="Name of example")
    parser.add_argument("--build-path", type=str, required=True, help="Path to build directory")
    parser.add_argument("--iterations", type=int, required=False, help="Iterations to run example")
    parser.add_argument("--output-path0", type=str, required=True, help="Path of output data")
    parser.add_argument("--output-path1", type=str, required=False, help="Path of output data (second location)")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument('-ppw', type=int, default=500, help='Points Per Window')
    parser.add_argument("-fs", type=str, default=250000000.0, help="Sampling Rate of Data. Check Config File.")
    parser.add_argument("-start-point", type=int, default=1000,help="Starting point of plots, must be multiple of number of samples.")
    parser.add_argument("-end-point", type=int, default=16000,help="Ending point of plots. ")
    parser.add_argument("-base-rx",type=str, default="rx_00", help="Base RX channel to measure against, format: rx_##")
    params = parser.parse_args(rest);
    if params.verbose:
        log.basicConfig(level=log.DEBUG)
    
    print("Running OARER RX Tests")
    # Make Output Directory
    if (os.path.exists(os.getcwd()+"/" + "temp") == False):
        os.mkdir(os.getcwd()+"/" + "temp" )
    test_ten_run(params, arch_params)