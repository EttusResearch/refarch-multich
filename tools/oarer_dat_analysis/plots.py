#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
import pylab as plt  
import numpy as np
from util import *
import logging as log
from usrpDat import *

def plot_samps(usrpDat: usrpDat, baseRX: usrpDat, directory: str, start_point: int, end_point: int):
    #Plot Real Samples
    log.info("Plotting Samples")
    subPlot1 = plt.subplot()
    i_samps,q_samps = usrpDat.deinterleave_iq()
    i_base_rx, q_base_rx = baseRX.deinterleave_iq()
    time_scale_samps = np.linspace(0,(len(usrpDat.data_array_full)/2)/baseRX.fs,(len(usrpDat.data_array_full)/2))
    time_scale_samps = time_scale_samps[start_point:]
    plt.tick_params(axis="both", labelsize=10)
    subPlot1.plot(time_scale_samps[:end_point], q_base_rx[:end_point]/(2**15-1), 'b')
    subPlot1.plot(time_scale_samps[:end_point], q_samps[:end_point]/(2**15-1), 'r')
    plt.title("Phase Aligned RX:"  + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=15)
    plt.legend(["Channel: " + baseRX.rx_channel_number, "Channel: " + usrpDat.rx_channel_number], fontsize=10)
    plt.ylabel("Amplitude (real)", fontsize=15)
    plt.xlabel("Time (us)", fontsize=15)
    plt.savefig(directory + "/samples/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot1.clear()
       
def plot_ptp_alignment(usrpDat: usrpDat, baseRX: usrpDat, alignment, directory: str, start_point: int, end_point: int):
    # Plot the alignment
    subPlot2 = plt.subplot()
    log.info("Plotting alignment")
    time_scale_alignment = np.linspace(0,(len(usrpDat.data_array_full)/2)/baseRX.fs,(len(usrpDat.data_array_full)/2))
    time_scale_alignment = time_scale_alignment[start_point:]
    subPlot2.plot(time_scale_alignment[:end_point], np.abs(alignment[usrpDat.rx_channel_number][:end_point]))
    plt.title("Phase Difference between Channels: " + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=12)
    plt.ylabel("Phase Delta (degrees)", fontsize=10)
    plt.xlabel("Time (us)", fontsize=10)
    plt.savefig(directory +"/ptp_alignment/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot2.clear()

def plot_ptp_average(usrpDat: usrpDat, baseRX: usrpDat, alignment, directory: str, start_point: int, end_point: int, pps: int):
     # Plot the mean alignment
    subPlot3 = plt.subplot()
    log.info("Plotting mean alignment")
    plt.tick_params(axis="x", labelsize=10)
    time_scale_start = np.linspace(0,(len(usrpDat.data_array_full)/2)/baseRX.fs,(len(usrpDat.data_array_full)/2))
    time_scale_start = time_scale_start[start_point:]
    time_scale_alignment = np.linspace(time_scale_start[0],time_scale_start[-1],len(alignment[usrpDat.rx_channel_number]))
    subPlot3.plot(time_scale_alignment, np.abs(alignment[usrpDat.rx_channel_number]))
    plt.title("Mean Phase Difference between Channels: " + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=12)
    plt.ylabel("Mean Phase Delta (degrees)", fontsize=10)
    plt.xlabel("Time (us)", fontsize=10)
    plt.savefig(directory +"/phase_average/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot3.clear()
    
