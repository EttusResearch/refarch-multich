"""
Plot samples from .dat file
"""

import numpy as np
import matplotlib.pyplot as plt
import argparse
import scipy.signal as signal 
import pyqtgraph as pg 
from PyQt5 import QtWidgets, uic
from pyqtgraph import PlotWidget, plot
import sys  # We need sys so that we can pass argv to QApplication
import os


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file-path", default="", type=str)
    parser.add_argument("-s", "--sample-rate", default=250000000, type=int)  
    parser.add_argument("-d", "--data-type", default="int16", type=str) 
    return parser.parse_args()

def deinterleave_iq(array = [], dFormat = "int16"):
    """Deinterleave a 1D array into two 1D arrays [0,1,2,3]>>>[[0,2],[1,3]]"""
    if (dFormat == "int16"):
        divisor = 32767.0
    else:
        divisor = 1 
    return [array[index::2]/divisor for index in range(2)]
    #Divide by int16 max value to normalize 



def main():
    """Plot samples from .dat file"""
    args = parse_args()

    data_array = np.fromfile(args.file_path, dtype=args.data_type)
    print(data_array[0])
    print(data_array[1])
    
    if (args.data_type == "int16"):
        i,q = deinterleave_iq(data_array, "int16")
        print(i[0])
        print(q[0])
       
    else:
        i,q = deinterleave_iq(data_array, "float32")
        print(i[0])
        print(q[0])
       
    data_array = data_array[0::2] + 1j*data_array[1::2]

    i_max = max(i)
    q_max = max(q)
    i_min = min(i)
    q_min = min(q)
    print("I Max: ", i_max)
    print("I Min: ", i_min)
    print("Q Max: " ,q_max)
    print("Q Min: ", q_min)

    n_taps = 64
    f_bw = 150000

    lpf = signal.remez(n_taps, [0, f_bw, f_bw+(args.sample_rate/2-f_bw)/4, args.sample_rate/2], [1,0], Hz = args.sample_rate)

    w, h = signal.freqz(lpf)
    y = signal.lfilter(lpf, 1.0, data_array)
    
    time_scale = np.linspace(0,len(i)/args.sample_rate,len(i))

    fig, ax = plt.subplots(3, 1)


    ax[0].plot(time_scale,i, time_scale, q)
    ax[0].set_xlabel('time')
    ax[0].set_ylabel('voltage')
    ax[1].specgram(data_array,NFFT = 1024, Fs = args.sample_rate)
    ax[1].set_xlabel('time')
    ax[1].set_ylabel('frequency')
    ax[1].set_ylim(-1000000,1000000)
    ax[2].psd(data_array, NFFT=1024, Fs = args.sample_rate, color = "blue")
    #ax[3].psd(y, NFFT=1024, Fs = args.sample_rate, color = "green")
    #plt.plot(time_scale,q)
    #plt.ylabel('voltage')
    #plt.xlabel('time')
    fig.tight_layout()
    plt.show()

    
    

if __name__ == "__main__":
    main()
