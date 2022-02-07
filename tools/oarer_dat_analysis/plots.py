
from matplotlib.pyplot import plot
import pylab as plt  
import numpy as np
from util import *
import logging as log
from usrpDat import *
import shutil
def plot_samps(usrpDat: usrpDat, baseRX: usrpDat):
    """
    Show a nice plot of samples 
    """
    
    subPlot = plt.subplot()
    i_samps,q_samps = usrpDat.deinterleave_iq()
    i_base_rx, q_base_rx = baseRX.deinterleave_iq()
    time_scale_samps = np.linspace(0,len(q_samps)/baseRX.fs,len(q_samps))
    plt.tick_params(axis="both", labelsize=10)
    # Plot the samples
    subPlot.plot(time_scale_samps, q_base_rx/(2**15-1), 'b')
    subPlot.plot(time_scale_samps, q_samps/(2**15-1), 'r')
    plt.title("Phase Aligned RX:"  + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=15)
    plt.legend(["Channel: " + baseRX.rx_channel_number, "Channel: " + usrpDat.rx_channel_number], fontsize=10)
    plt.ylabel("Amplitude (real)", fontsize=15)
    plt.xlabel("Time (us)", fontsize=15)
    plt.savefig("temp/samples/"+ "samples_" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot.clear()
       
def plot_alignment(usrpDat: usrpDat, baseRX: usrpDat, alignment):

    
    # Plot the alignment
    subPlot = plt.subplot()
    log.info("Plotting alignment")
    time_scale_alignment = np.linspace(0,len(alignment[usrpDat.rx_channel_number])/baseRX.fs,len(alignment[usrpDat.rx_channel_number]))
    subPlot.plot(time_scale_alignment, np.abs(alignment[usrpDat.rx_channel_number]))
    plt.title("Phase Difference between Channels: " + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=15)
    plt.ylabel("Phase Delta (degrees)", fontsize=15)
    plt.xlabel("Time (us)", fontsize=15)
    #plt.ylim([-np.pi, np.pi])
    plt.savefig("temp/alignment/"+"alignment_" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot.clear()