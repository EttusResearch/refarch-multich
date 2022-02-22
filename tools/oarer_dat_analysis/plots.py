
from matplotlib import pyplot
from matplotlib.ticker import FormatStrFormatter
import pylab as plt  
import numpy as np
from util import *
import logging as log
from usrpDat import *
import shutil

def plot_samps(usrpDat: usrpDat, baseRX: usrpDat, directory: str, start_point: int, end_point: int):
    """
    Show a nice plot of samples 
    """
    
    subPlot = plt.subplot()
    i_samps,q_samps = usrpDat.deinterleave_iq()
    i_base_rx, q_base_rx = baseRX.deinterleave_iq()
    time_scale_samps = np.linspace(0,len(q_samps)/baseRX.fs,len(q_samps))
    plt.tick_params(axis="both", labelsize=10)
    # Plot the samples
    subPlot.plot(time_scale_samps[start_point:end_point], q_base_rx[start_point:end_point]/(2**15-1), 'b')
    subPlot.plot(time_scale_samps[start_point:end_point], q_samps[start_point:end_point]/(2**15-1), 'r')
    plt.title("Phase Aligned RX:"  + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=15)
    plt.legend(["Channel: " + baseRX.rx_channel_number, "Channel: " + usrpDat.rx_channel_number], fontsize=10)
    plt.ylabel("Amplitude (real)", fontsize=15)
    plt.xlabel("Time (us)", fontsize=15)
    plt.savefig(directory + "/samples/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot.clear()
       
def plot_ptp_alignment(usrpDat: usrpDat, baseRX: usrpDat, alignment, directory: str, start_point: int, end_point: int):

    
    # Plot the alignment
    subPlot = plt.subplot()
    log.info("Plotting alignment")
    time_scale_alignment = np.linspace(0,len(alignment[usrpDat.rx_channel_number])/baseRX.fs,len(alignment[usrpDat.rx_channel_number]))
    subPlot.plot(time_scale_alignment[start_point:end_point], np.abs(alignment[usrpDat.rx_channel_number][start_point:end_point]))
    plt.title("Phase Difference between Channels: " + baseRX.rx_channel_number + " to " + usrpDat.rx_channel_number, fontsize=15)
    plt.ylabel("Phase Delta (degrees)", fontsize=15)
    plt.xlabel("Time (us)", fontsize=15)
    #plt.ylim([-np.pi, np.pi])
    plt.savefig(directory +"/ptp_alignment/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)
    subPlot.clear()

def plot_phase_diffs_composite(usrpDat: usrpDat, baseRX: usrpDat,  directory: str, start_point: int, end_point: int, phase_diff, phases):
    i_samps,q_samps = usrpDat.deinterleave_iq()
    i_base_rx, q_base_rx = baseRX.deinterleave_iq()
    time_scale_samps = np.linspace(0,len(q_samps)/baseRX.fs,len(q_samps))
    fig, axs = plt.subplots(3, 1)
    axs[0].set_title("Input Signals")
    axs[0].plot(i_base_rx)
    axs[0].plot(i_samps)
    axs[1].set_title("Phases")
    axs[1].plot(phases[baseRX.rx_channel_number])
    axs[1].plot(phases[usrpDat.rx_channel_number])
    axs[2].set_title("Phase Difference")
    axs[2].plot(phase_diff[usrpDat.rx_channel_number])
    plt.savefig(directory +"/phase_composite/" + baseRX.rx_channel_number + "_to_" + usrpDat.rx_channel_number +".png",)