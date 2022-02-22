#!/usr/bin/env python3


from cmath import phase
from os import fsdecode
import sys
from pprint import pprint
import logging as log
from plots import *
from util import *
from usrpDat import *
import configargparse, argparse

def main():
    args = parse_args()

    if args.verbose:
        log.basicConfig(level=log.DEBUG)

    if args.filename: 
        log.info("File {} selected.".format(args.filename))
        filesize = os.path.getsize(args.filename)
        iq_data = get_iq_object(args.filename, args.format, filesize)
    elif args.folder:
        log.info("Reading in folder: {}.".format(args.folder))
        folder = get_newest_folder(args.folder)
        iq_data_dict = {}
        complex_dict = {}
        for file in os.listdir(folder):
            log.info("File {} selected.".format(file))
            filesize = os.path.getsize(str(folder) + '/' + file)
            iq_data = get_iq_object(file, args.format, filesize, args)
            print(iq_data.filename)
            iq_data.folder = str(folder)
            iq_data.fs = float(args.fs)
            iq_data.read_samples(args.format)
            iq_data.deinterleave_iq()
            iq_data.convert_to_complex()
            iq_data_dict[iq_data.rx_channel_number] = iq_data
            # Convert data to complex and save to dict. 
            complex_dict[iq_data.rx_channel_number] = iq_data.complex_data 
    alignment = calculate_ptp_alignment_all(iq_data_dict, args.base_rx)
    phase_diff, phases = calculate_phase_diff(complex_dict, args.base_rx, args.fs, args.pps)
    create_plot_directories("temp")
    for data in alignment:
        plot_ptp_alignment(iq_data_dict[data], iq_data_dict[args.base_rx], alignment, "temp", args.start_point, args.end_point)
    for data in iq_data_dict:
        if data != args.base_rx:
            plot_samps(iq_data_dict[data], iq_data_dict[args.base_rx], "temp",  args.start_point, args.end_point)
            plot_phase_diffs_composite(iq_data_dict[data], iq_data_dict[args.base_rx], "temp", args.start_point, args.end_point, phase_diff, phases)

    

if __name__ == "__main__":
    sys.exit(not main())