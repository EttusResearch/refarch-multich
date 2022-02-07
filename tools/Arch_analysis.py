#!/usr/bin/env python3
#
# Copyright 2018,2019 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""
UHD Phase Alignment: Phase alignment test using the UHD Python API.
"""


import argparse
from datetime import datetime, timedelta
import itertools as itt
import sys
import time
import logging
from matplotlib.pyplot import plot
import numpy as np
import numpy.random as npr
from paramiko import Channel
#print(sys.path)
import uhd
import configargparse
import glob
import os
import shutil
import re
from scipy.fft import fft, fftfreq, rfft   
import matplotlib.mlab as mlb
import iqtools
 
CLOCK_TIMEOUT = 1000  # 1000mS timeout for external clock locking
INIT_DELAY = 0.05  # 50mS initial delay before transmit
CMD_DELAY = 0.05  # set a 50mS delay in commands
NUM_RETRIES = 10  # Number of retries on a given trial before giving up

def split_args(args_str):
    """
    Split a string of the form key1=value1,key2=value2 into a dict of the form
    {key1 => 'value1', key2 => 'value2'}.
    """
    return {
        x.split('=', 1)[0].strip(): x.split('=', 1)[1]
        for x in args_str.split(",")
        if x
    }


def parse_args():
    """Parse the command line arguments"""
    description = """TBD
    """
    parser = configargparse.ArgParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description, default_config_files=['~/*.conf'])
    parser.add_argument("-file-path", default=None, type=str, help="Directory that the samples are located in.")
    parser.add_argument("--drift-threshold", type=float, default=2.,
                        help="Maximum frequency drift (deg) while testing a given frequency")
    parser.add_argument("--stddev-threshold", type=float, default=2.,
                        help="Maximum frequency deviation (deg) over a single receive call")
    parser.add_argument("-s", "--sample_rate", default=1, type=int)
    parser.add_argument("-n", "--newest_file", type=str)
    parser.add_argument("-c","--base-rx",type=str, default="rx_00", help="Base RX channel to measure against, format: rx_##")
    # Extra, advanced argument
    parser.add_argument("--plot", default=False, action="store_true",
                        help="Plot results")
    args = parser.parse_args()
    

    return args


class LogFormatter(logging.Formatter):
    """Log formatter which prints the timestamp with fractional seconds"""
    @staticmethod
    def pp_now():
        """Returns a formatted string containing the time of day"""
        now = datetime.now()
        return "{:%H:%M}:{:05.2f}".format(now, now.second + now.microsecond / 1e6)

    def formatTime(self, record, datefmt=None):
        converter = self.converter(record.created)
        if datefmt:
            formatted_date = converter.strftime(datefmt)
        else:
            formatted_date = LogFormatter.pp_now()
        return formatted_date


def window(seq, width=2):
    """Returns a sliding window (of `width` elements) over data from the iterable.
    s -> (s0,s1,...s[n-1]), (s1,s2,...,sn), ...
    Itertools example found at https://docs.python.org/release/2.3.5/lib/itertools-example.html
    """
    seq_iter = iter(seq)
    result = tuple(itt.islice(seq_iter, width))
    if len(result) == width:
        yield result
    for elem in seq_iter:
        result = result[1:] + (elem,)
        yield result


def generate_time_spec(usrp, time_delta=0.05):
    """Return a TimeSpec for now + `time_delta`"""
    return usrp.get_time_now() + uhd.types.TimeSpec(time_delta)


def plot_samps(samps_dict, alignment_dict, args):
    """
    Show a nice plot of samples and their phase alignment
    """
    try:
        import pylab as plt
    except ImportError:
        logger.error("--plot requires pylab.")
        return
    subPlot = plt.subplot()
    for key in alignment_dict:
        
        i_samps,q_samps = deinterleave_iq(samps_dict[key])
        i_base_rx, q_base_rx = deinterleave_iq(samps_dict[args.base_rx])
        time_scale_samps = np.linspace(0,len(q_samps)/args.sample_rate,len(q_samps))
        plt.tick_params(axis="both", labelsize=10)
        # Plot the samples
        subPlot.plot(time_scale_samps, q_base_rx/(2**15-1), 'b')
        subPlot.plot(time_scale_samps, q_samps/(2**15-1), 'r')
        plt.title("Phase Aligned RX:"  + args.base_rx + " to " + key, fontsize=15)
        plt.legend(["Channel: " + args.base_rx, "Channel: " + key], fontsize=10)
        plt.ylabel("Amplitude (real)", fontsize=15)
        plt.xlabel("Time (us)", fontsize=15)
        plt.savefig("temp/"+ "samples_" + args.base_rx + "_to_" + key +".png",)
        subPlot.clear()
        #plt.show()
        # Plot the alignment
        logger.info("plotting alignment")
        time_scale_alignment = np.linspace(0,len(alignment_dict[key])/args.sample_rate,len(alignment_dict[key]))
        subPlot.plot(time_scale_alignment, np.abs(alignment_dict[key]))
        plt.title("Phase Difference between Channels: " + args.base_rx + " to " + key, fontsize=15)
        plt.ylabel("Phase Delta (degrees)", fontsize=15)
        plt.xlabel("Time (us)", fontsize=15)
        #plt.ylim([-np.pi, np.pi])
        plt.savefig("temp/"+"alignment_" + args.base_rx + "_to_" + key +".png",)
        subPlot.clear()
        
        #plt.show()


def calc_max_drift(phase_vals):
    """Returns the maximum drift (radians) between the mean phase values of runs
    This works for all values, even those around +/-pi.
    For example, calc_max_drift([179 degrees, -179 degrees]) = 2 degrees, not
    358 degrees"""
    def span(ll):
        "Return max - min of values in ll"
        return max(ll) - min(ll)
    # Roll over negative values up to above pi, see if that improves things
    # Ensure that phase_vals is a numpy array so that we can use nifty indexing
    # below
    phase_vals = np.array(phase_vals)
    norm_span = span(phase_vals)
    corrected = phase_vals + (phase_vals < 0)*2*np.pi
    corr_span = span(corrected)
    return corr_span if corr_span < norm_span else norm_span


def check_results(alignment_stats, drift_thresh, stddev_thresh):
    """Print the alignment stats in a nice way
    alignment_stats should be a dictionary of the following form:
    {test_freq : [list of runs], ...}
    ... the list of runs takes the form:
    [{dictionary of run statistics}, ...]
    ... the run dictionary has the following keys:
    mean, stddev, min, max, test_freq, run_freq
    ... whose values are all floats
    """
    success = True  # Whether or not we've exceeded a threshold
    msg = ""
    for freq, stats_list in sorted(alignment_stats.items()):
        band_success = True
        # Try to grab the test frequency for the frequency band
        try:
            test_freq = stats_list[0].get("test_freq")
        except (KeyError, IndexError):
            test_freq = 0.
            logger.error("Failed to find test frequency for test band %.2fMHz", freq)
        msg += "=== Frequency band starting at {:.2f}MHz. ===\n".format(freq/1e6)
        msg += "Test Frequency: {:.2f}MHz ===\n".format(test_freq/1e6)

        # Allocate a list so we can calulate the drift over a set of runs
        mean_list = []

        for run_dict in stats_list:
            run_freq = run_dict.get("run_freq", 0.)
            # Convert mean and stddev to degrees
            mean_deg = run_dict.get("mean", 0.) * 180 / np.pi
            stddev_deg = run_dict.get("stddev", 0.) * 180 / np.pi
            if stddev_deg > stddev_thresh:
                band_success = False
                success = False

            msg += "{:.2f}MHz<-{:.2f}MHz: {:.3f} deg +- {:.3f}\n".format(
                test_freq/1e6, run_freq/1e6, mean_deg, stddev_deg
            )
            mean_list.append(mean_deg)

        # Report the largest difference in mean values of runs
        max_drift = calc_max_drift(mean_list)
        if max_drift > drift_thresh:
            band_success = False
            success = False
        msg += "--Maximum drift over runs: {:.2f} degrees\n".format(max_drift)
        if band_success is False:
            msg += "Failure!\n"
        # Print a newline to separate frequency bands
        msg += "\n"

    logger.info("Printing statistics!\n%s", msg)
    return success

def measure_phase(samps0, samps1):
    alignment = np.angle(np.conj(samps0) * samps1, True)
    return alignment

def get_fft_freqs(data):
    n = data.size 

def measure_phase_single_tone(samps0, samps1):
    #
    try:
        import pylab as plt
    except ImportError:
        logger.error("--plot requires pylab.")
        return
    yf_0 = fft(samps0)
    xf_0 = fftfreq(len(samps0), 1/33330000)
    #yf_1 = rfft(np.real(samps1))
    #xf_1 = fftfreq(8001, 1/33330000)
    plt.plot(xf_0,abs(yf_0))
    plt.xlim(-750000,750000)
    max_y = max(yf_0)
    max_x = xf_0[yf_0.argmax()]
    print (max_x, max_y)
    plt.savefig("temp/"+ "fft_" + "test" + "_to_" + "key" +".png",)

def deinterleave_iq(array = [], *args):
    """Deinterleave a 1D array into two 1D arrays [0,1,2,3]>>>[[0,2],[1,3]]"""
    return [array[index::2] for index in range(2)]

def samps_vector(directory, args):
    data_dict = {}
    fileFolder = directory
    data_array = []
    
    complex_dict = {}
    if directory is None:
        folders = glob.glob(args.newest_file+"*")
        fileFolder = max(folders, key=os.path.getctime)
    print(fileFolder)
    for file in os.listdir("/"+fileFolder):
        if file.endswith(".dat"):
            complex_array = []
            data_array = np.fromfile(fileFolder+"/"+file, dtype=np.int16)
            i, q = deinterleave_iq(data_array)
            for x in range(0,len(i)):
                complex_array.append(np.complex(q[x], i[x]))

            rx_channel = extract_channel_rx(file)
            data_dict[rx_channel] = data_array
            complex_dict[rx_channel] = complex_array
    return data_dict, complex_dict

def extract_channel_rx(filename):
    """Look in filename for rx channel"""
    match_rx= re.search("rx_[0-9][0-9]", filename)
    rx_channel_number = match_rx.group()
    return rx_channel_number

def extract_channel_tx(filename):
    """Look in filename for tx channel"""
    match_tx= re.search("tx_[0-9][0-9]", filename)
    tx_channel_number = match_tx.group() 
    return tx_channel_number


def main():

    args = parse_args()
    alignment_dict = {}
    if (os.path.exists(os.getcwd()+"/temp")):
        shutil.rmtree( os.getcwd()+"/temp" )
    os.mkdir(os.getcwd()+"/temp" )
    data_dict, complex_dict = samps_vector(args.file_path, args)
    for key in data_dict:
        if key != args.base_rx:
            logger.info("Calculating Alignment: " + args.base_rx + " to " + key)
            alignment = measure_phase(complex_dict[args.base_rx][100:],complex_dict[key][100:])
            alignment_dict[key] = alignment
    
    #plot_samps(data_dict,alignment_dict,args)
    measure_phase_single_tone(complex_dict[args.base_rx], complex_dict['rx_01'])
    #file_size = os.path.getsize(fileFolder+"/"+file)
    #nsamps = file_size/4
    #data_array = np.fromfile(fileFolder+"/"+file, dtype=np.int16)
    #i,q = deinterleave_iq(data_array["rx_03"])
    #i = i[600:16000]
    #q = q[600:16000]
    #time_scale = np.linspace(600,len(i)/args.sample_rate,len(i))
    #print(alignment[1000:1500])

    #return check_results(all_alignment_stats, args.drift_threshold, args.stddev_threshold)

    
    

if __name__ == "__main__":
     # Setup the logger with our custom timestamp formatting
    global logger
    logger = logging.getLogger(__name__)
    logger.setLevel(logging.DEBUG)
    console = logging.StreamHandler()
    logger.addHandler(console)
    formatter = LogFormatter(fmt='[%(asctime)s] [%(levelname)s] %(message)s')
    console.setFormatter(formatter)
    #Call Main
    sys.exit(not main())