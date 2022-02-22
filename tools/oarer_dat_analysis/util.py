from email.mime import base
import os
import logging as log
import numpy as np
import glob 
import configargparse, argparse
import shutil
from usrpDat import *
from usrpDat import usrpDat
from util import *
import os

def parse_args():
    description = "TBD"
    parser = configargparse.ArgParser(formatter_class=argparse.RawTextHelpFormatter,
                                     description=description, default_config_files=['~/*.conf'])
    parser.add_argument("-filename", type=str, default=None,help="Name of the input file.")
    parser.add_argument("-folder", type=str, default=None,  help="Folder to search for newest .dat files.")
    parser.add_argument("-fs", type=str, default=33330000.0, help="Sampling Rate of Data.")
    parser.add_argument("-start-point", type=int, default=0,help="Starting point of plots")
    parser.add_argument("-end-point", type=int, default=0,help="Ending point of plots")
    parser.add_argument("-format", type=str, default="int16", help="Data Format, default: np.int16")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument("-base-rx",type=str, default="rx_00", help="Base RX channel to measure against, format: rx_##")
    parser.add_argument('-l', type=int,  default=1024, help='Frame-length - default is 1024')
    parser.add_argument('-n', type=int, default=10, help='Number of frames - default 10')
    parser.add_argument('-s', type=int, default=1, help='Starting frame - default is 1')
    parser.add_argument('-pps', type=int, default=500, help='Points Per Segment')
    args = parser.parse_args()
    return args

def get_iq_object(filename, datatype, filesize, args):
   
    _, file_extension = os.path.splitext(filename)
    
    if file_extension.lower() == '.dat':
        log.info('This is a USRP .dat file.')
        iq_data = usrpDat(filename = filename, datatype=datatype , filesize = filesize, args =  args)
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
        
def calculate_ptp_alignment_all(usrpDataDict: dict, baseRX):
    result = {}
    for key in usrpDataDict:       
        if key != baseRX:
            log.info("Calculating Alignment: " + baseRX + " to " + key)
            alignment = np.angle(np.conj(usrpDataDict[baseRX].complex_data) * usrpDataDict[key].complex_data, True)
            result[key] = alignment
    return result

def create_plot_directories(identifier: str):
    # Create temp directory if it does not exist.
    if identifier:
        identifier = identifier
    else:
        identifier = ''
    if (os.path.exists(os.getcwd()+"/" + identifier)):
        shutil.rmtree( os.getcwd()+"/" + identifier )
    os.mkdir(os.getcwd()+"/" + identifier )
    os.mkdir(os.getcwd()+"/" + identifier + "/ptp_alignment/")
    os.mkdir(os.getcwd()+"/" + identifier + "/samples/")
    os.mkdir(os.getcwd()+"/" + identifier + "/phase_composite/")

def calculate_phase_diff(usrpDataDict: dict, baseRX, fs, points_per_segment):
    phase_diff = {}
    phases = {}
    test = ToneMeasurement()
    phases[baseRX] = test.extract_phase_over_time(usrpDataDict[baseRX], float(fs), points_per_segment)
    for key in usrpDataDict:       
        if key != baseRX:
            log.info("Calculating Phases: " + baseRX + " to " + key)
            phases[key] = test.extract_phase_over_time(usrpDataDict[key], float(fs), points_per_segment)
            diff = abs(phases[key] - phases[baseRX])
            phase_diff[key] = diff
    
    return phase_diff, phases