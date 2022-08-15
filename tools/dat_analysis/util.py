
#
#  Copyright 2021-2022 Ettus Research, a National Instruments Brand
#
#  SPDX-License-Identifier: GPL-3.0-or-later
#

from asyncio.subprocess import PIPE
from cProfile import run
from email.mime import base
import os
import logging as log
import numpy as np
import glob 
import configargparse, argparse
import shutil

from sklearn.model_selection import KFold
from usrpDat import *
from usrpDat import usrpDat
from util import *
import os
import subprocess
import pathlib
import plots as plotutil
from sklearn import preprocessing


def parse_args():
    description = "This utility allows for the viewing of dat files generated by the MRFRA architecture."
    parser = configargparse.ArgParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description, default_config_files=['~/*.conf'])
    parser.add_argument("-filename", type=str, default=None,help="Name of the input file.")
    parser.add_argument("-folder0", type=str, required=True, default=None,  help="First folder to search for newest .dat files or parent directory for use with -n flag")
    parser.add_argument("-folder1", type=str, required=False,  help="Second folder to search for newest .dat files or parent directory for use with -n flag")
    parser.add_argument("-n", action="store_true", help="Import the newest folder(s) from -folder and/or -folder1")
    parser.add_argument("-numfolders", type=int, default=1, help="X number of new folders to import, for use with -n flag.")
    parser.add_argument("-fs", type=str, default=33330000.0, help="Sampling Rate of Data.")
    parser.add_argument("-start-point", type=int, default=0,help="Starting point of plots, must be multiple of number of samples.")
    parser.add_argument("-end-point", type=int, default=0,help="Ending point of plots")
    parser.add_argument("-format", type=str, default="int16", help="Data Format, default: int16")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-base-rx",type=str, default="rx_00", help="Base RX channel to measure against, format: rx_##")
    parser.add_argument("-signal-freq", type=int, required=False, help="Frequency of transmitted sinusoid. Automatically calculates ppw.")
    parser.add_argument('-ppw', type=int, help='Points Per Window, optional, will override value calculated with signal-freq.')
    args = parser.parse_args()
    return args

def calculate_window(signal_freq: int, fs):
    T = 1/signal_freq # Period
    two_T = 2*T # two Periods
    samples = float(fs)*two_T # Samples for two periods
    return int(samples)

def get_iq_object(filename, datatype, filesize):
    _, file_extension = os.path.splitext(filename)
    if file_extension.lower() == '.dat':
        log.info('This is a USRP .dat file.')
        iq_data = usrpDat(filename = filename, datatype=datatype , filesize = filesize)
    else:
        log.info('Please use a valid .dat filetype.')   
    return iq_data

def get_last_num_folders(mnt, numfolders):
    folder_list = []
    for x in range(1,numfolders+1):
        folders = sorted(pathlib.Path(mnt).glob('*/'), key=os.path.getmtime)[-x]
        folder_list.append(folders)
    return folder_list
        
def calculate_ptp_alignment_all(usrpDataDict: dict, baseRX, points_per_window):
    result = {}
    average_alignment = {}
    match_base_tx= re.search("tx_[0-9][0-9]_", baseRX)
    base_tx_channel_number = match_base_tx.group()
    match_base_run= re.search("_run_[0-9][0-9]",baseRX)
    base_run_number = match_base_run.group()
    for key in usrpDataDict:
        match_tx= re.search("tx_[0-9][0-9]_", key)
        tx_channel_number = match_tx.group()
        match_run= re.search("_run_[0-9][0-9]", key)
        run_number = match_run.group()
        if key != baseRX and tx_channel_number == base_tx_channel_number and run_number == base_run_number:
            log.info("Calculating Alignment: " + baseRX + " to " + key)
            alignment = np.angle(np.conj(usrpDataDict[baseRX].complex_data) * usrpDataDict[key].complex_data, True)
            mean_alignment = np.mean(alignment)
            #alignment -= mean_alignment
            result[key] = alignment
            #print(alignment)
            #print(np.mean(alignment))
            #result[key] = preprocessing.normalize(alignment[:,np.newaxis], axis=0, copy=False).ravel()
            average_alignment[key] = np.average(alignment.reshape(-1, points_per_window), axis=1)
    return result, average_alignment


def batch_folder_import( path0: str , ppw, new = True, numfolders = 1, datatype = "int16", start_point = 1000, fs = 33330000.0):
    folderlist = []
    log.info("Reading in folder: {}.".format(path0))
    folderlist= get_last_num_folders(path0, numfolders)
    path_folderdict = {} #dicts with run folders as keys, contain dicts of data with RX channels as keys
    # Read in data from path1
    for folder in folderlist:
        # IQ Data Dicts with RX channels as keys
        file_data_dict ={}
        for file in os.listdir(folder):
            filesize = os.path.getsize(str(folder) + '/' + file)
            # IQ Data for file
            iq_data = get_iq_object(file, datatype, filesize)
            iq_data.folder = str(folder)
            iq_data.fs = float(fs)
            iq_data.read_samples(datatype, start_point)
            iq_data.determine_nsamps()
            if (iq_data.nsamples - start_point) % ppw != 0:
                x = (iq_data.nsamples-start_point) % ppw
                sys.exit("ERROR: PPW ("+str(ppw)+") : "+"\nIncrease samples by: " + str(x) + " or use start_point = " + str(start_point+x))
            iq_data.deinterleave_iq()
            iq_data.convert_to_complex()
            channel_string = iq_data.tx_channel_number + iq_data.rx_channel_number + iq_data.run_number
            file_data_dict[channel_string] = iq_data
            path_folderdict[folder] = file_data_dict
    return path_folderdict

def batch_analyze_plot(dict0 : dict, ppw,  baseRX , save_directory = None, start_point = 0, end_point = 100):
    alignment_dict = {}
    avg_alignment_dict = {}
    for k_folder, v_folder in dict0.items():
        # dict0, alignment_dict, avg_alignment_dict are dictionaries of dictionairies. 
        # Keys are the path to the test directory and data
        tx_list, rx_list, run_list = extract_file_info(v_folder)
        if save_directory == None:
            create_plot_directories(str(k_folder))
        else:
            fname = pathlib.PurePath(k_folder)
            #os.mkdir(save_directory + "/" +str(fname.name))
            create_plot_directories(save_directory + "/" +str(fname.name))
        for tx in tx_list:
            for run in run_list:
                base_channel_string = tx + baseRX + run
                #alignment_dict[k_folder], avg_alignment_dict[k_folder] = calculate_ptp_alignment_all(v_folder, base_channel_string, ppw)
                if save_directory != "":
                    image_output = str(k_folder)
                else:
                    image_output = save_directory
                #for data in avg_alignment_dict[k_folder]:
                    ##plotutil.plot_ptp_average(dict0[k_folder][data], dict0[k_folder][base_channel_string], avg_alignment_dict[k_folder], image_output , start_point)
                #for data in alignment_dict[k_folder]:
                    ##plotutil.plot_ptp_alignment(dict0[k_folder][data], dict0[k_folder][base_channel_string], alignment_dict[k_folder], image_output , start_point, end_point)
                for data in dict0[k_folder]:
                    if data != baseRX:
                        plotutil.plot_samps(dict0[k_folder][data], dict0[k_folder][base_channel_string], image_output,  start_point, end_point) 
    
def create_plot_directories(identifier: str):
    # Create temp directory if it does not exist.
    log.info("Creating Directories")
    os.mkdir(identifier + "/ptp_alignment/")
    os.mkdir(identifier + "/samples/")
    os.mkdir(identifier + "/phase_average/")

def extract_file_info(filedict: dict):
    # Pulls out tx, rx, and run from file name if needed
    iq_object_keys = list(filedict.keys())
    tx_list = []
    run_list = []
    rx_list = []
    for key in iq_object_keys:
        match_tx= re.search("tx_[0-9][0-9]_", key)
        tx_channel_number = match_tx.group() 
        if tx_channel_number not in tx_list:
            tx_list.append(tx_channel_number)
        match_run= re.search("_run_[0-9][0-9]", key)
        run_number = match_run.group() 
        if run_number not in run_list:
            run_list.append(run_number)
        match_rx= re.search("rx_[0-9][0-9]", key)
        rx_channel_number = match_rx.group()
        if rx_channel_number not in rx_list:
            rx_list.append(rx_channel_number)
    return tx_list, rx_list, run_list

def move_data(params, new, iterations, temp_location, d1):
    # Move data to temp folder for analysis
    # Will move iterations number of folders
    folder_list0 = []
    folder_list1 = []
    if new:
        folder_list0 = get_last_num_folders(params.folder0, iterations)
    if new and params.folder1:
        folder_list1 = get_last_num_folders(params.folder1, iterations)

    if new == False and params.folder1 == False:
        folder_list0.append(params.folder0)
    elif new == False:
        folder_list0.append(params.folder0)
        folder_list1.append(params.folder1)
    for folder in folder_list0:
        log.info(folder)
        src0 = folder
        basename = os.path.basename(src0)
        dst0 = os.path.join(temp_location,basename)
        try:
            os.mkdir(dst0)
        except:
            print("Directory Exists")
        for file in os.listdir(src0):
            os.chdir(src0)
            shutil.copy2(file,os.path.join(dst0,""))        
    if params.folder1:
        for folder1 in folder_list1:
            log.info(folder1)
            src1 = folder1
            os.chdir(src1)
            for file in os.listdir(src1):
                shutil.copy2(file, dst0)
    print("Received Files moved to: {} ".format(temp_location))
    return temp_location