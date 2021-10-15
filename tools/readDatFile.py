#!/usr/bin/env python3

"""
Copyright 2010-2012,2014-2015 Ettus Research LLC
Copyright 2021 Ettus Research, a National Instruments Company
SPDX-License-Identifier: GPL-3.0-or-later

Plot samples from .dat file. 
DISCLAIMER: This is meant as an example and not as a analysis tool. 
It is useful for determining if the data being received is noise or a signal. 

"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import os


def parse_args():
    """
    Parse the command line arguments
    Example command line syntax:
    python3 readDatFile.py -f <directory where files are> -s <rx sampling rate>
    python3 readDatFile.py -f "/mnt/md0/CW_2.000000_GHz_9282021_113939_test.dat" -s 33330000
    """

    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file-path", default="", type=str)
    parser.add_argument("-s", "--sample-rate", default=1, type=int)
    return parser.parse_args()

def deinterleave_iq(array = [], *args):
    """Deinterleave a 1D array into two 1D arrays [0,1,2,3]>>>[[0,2],[1,3]]"""
    return [array[index::2] for index in range(2)]

def main():
    """Plot samples from .dat file"""
    args = parse_args()
    subPlot = plt.subplot()
    for file in os.listdir("/"+args.file_path):
        if file.endswith(".dat"):
            data_array = np.fromfile(args.file_path+"/"+file, dtype=np.int16)
            i,q = deinterleave_iq(data_array)
            time_scale = np.linspace(0,len(i)/args.sample_rate,len(i))
            subPlot.plot(time_scale,i/(2**15-1))
            subPlot.plot(time_scale,q/(2**15-1))
            plt.ylabel('voltage')
            plt.xlabel('time')
            plt.savefig("delme"+file+".png")
            subPlot.clear()
            

if __name__ == "__main__":
    main()
