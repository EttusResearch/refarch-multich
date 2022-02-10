
from matplotlib.pyplot import plot
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

def plot_spectrum(usrpDat: usrpDat, f, p, directory, cen=0.0, span=None, dbm=False, title='Spectrum'):
    """Plot average power in dBm per Hz"""

    if not span:
        mask = (f != 0) | (f == 0)
    else:
        mask = (f <= span / 2) & (f >= -span / 2)
    if dbm:
        plt.plot(f[mask], usrpDat.get_dbm(p[mask]))
    else:
        plt.plot(f[mask], p[mask])

    ax = plt.gca()
    ax.xaxis.set_major_formatter(FormatStrFormatter('%.0e'))

    plt.xlabel("Delta f [Hz] @ {}".format(get_eng_notation(cen, 'Hz')))
    plt.title(title)
    if dbm:
        plt.ylabel('Power Spectral Density [dBm/Hz]')
    else:
        plt.ylabel('Power Spectral Density')

    #plt.xlim(0,2800000000)
    plt.grid(True)
    plt.savefig(directory + "/spectrum/" + usrpDat.rx_channel_number + '.png')  # , bbox_inches='tight')
    plt.close()