"""
Plot samples from .dat file
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse


def parse_args():
    """
    Parse the command line arguments
    Example command line syntax:
    python readDatFile.py -f "/mnt/md0/CW_2.000000_GHz_9132021_10360_test.dat/test.tx_00_rx_01_run_00_cw_2e+09_thread_0.dat" -s 250000000
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

    data_array = np.fromfile(args.file_path, dtype=np.int16)
    i,q = deinterleave_iq(data_array)
    time_scale = np.linspace(0,len(i)/args.sample_rate,len(i))

    plt.plot(time_scale,i)
    plt.plot(time_scale,q)
    plt.ylabel('voltage')
    plt.xlabel('time')
    plt.savefig("delme.png")

if __name__ == "__main__":
    main()
