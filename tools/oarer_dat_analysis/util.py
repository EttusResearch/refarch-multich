from email.mime import base
import os
import logging as log
from scipy.signal import hilbert
from scipy.io import wavfile
import xml.etree.ElementTree as et
import numpy as np
import glob 
import types
import uproot3
import uproot3_methods.classes.TH1
import configargparse, argparse

from usrpDat import *
from util import *

def parse_args():
    description = "TBD"
    parser = configargparse.ArgParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description, default_config_files=['~/*.conf'])
    parser.add_argument("-filename", type=str, default=None,help="Name of the input file.")
    parser.add_argument("-folder", type=str, default=None, help="Folder to search for newest .dat files.")
    parser.add_argument("-fs", type=str, default=33330000.0, help="Sampling Rate of Data.")
    parser.add_argument("-format", type=str, default="int16", help="Data Format, default: np.int16")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-base-rx",type=str, default="rx_00", help="Base RX channel to measure against, format: rx_##")
    args = parser.parse_args()
    return args

def get_iq_object(filename, datatype, filesize):
   
    _, file_extension = os.path.splitext(filename)
    
    if file_extension.lower() == '.dat':
        log.info('This is a USRP .dat file.')
        iq_data = usrpDat(filename = filename, datatype=datatype , filesize = filesize)
    else:
        log.info('Please use a valid .dat filetype.')
        
    return iq_data

def get_newest_folder(mnt):
    #Search directory for newest folder 
    folders = glob.glob(mnt+"*")
    fileFolder = max(folders, key=os.path.getctime)
    return fileFolder

def get_file_list(folder):
    dir_list = os.listdir(folder)
    return dir_list
        
def calculate_ptp_alignment(usrpData, baseRX):
    alignment = np.angle(np.conj(baseRX) * usrpData, True)
    return alignment

def calculate_ptp_alignment_all(usrpDataDict: dict, baseRX):
    result = {}
    for key in usrpDataDict:       
        if key != baseRX:
            print(key)
            log.info("Calculating Alignment: " + baseRX + " to " + key)
            alignment = np.angle(np.conj(usrpDataDict[baseRX].data_array) * usrpDataDict[key].data_array, True)
            result[key] = alignment
    return result