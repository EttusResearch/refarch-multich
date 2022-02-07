#!/usr/bin/env python3


from os import fsdecode
import sys
from pprint import pprint
import logging as log
from plots import *
from util import *
from usrpDat import *


def main():
    scriptname = 'OARERAnalysis'
    args = parse_args()

    if args.verbose:
        log.basicConfig(level=log.DEBUG)

    if args.filename: 
        log.info("File {} selected.".format(args.filename))
        iq_data = get_iq_object(args.filename, args.format)
    elif args.folder:
        log.info("Reading in folder: {}.".format(args.folder))
        folder = get_newest_folder(args.folder)
        iq_data_dict = {}
        for file in os.listdir(folder):
            log.info("File {} selected.".format(file))
            filesize = os.path.getsize(str(folder) + '/' + file)
            iq_data = get_iq_object(file, args.format, filesize)
            iq_data.folder = str(folder)
            iq_data.fs = float(args.fs)
            iq_data.read_samples(args.format)
            print(iq_data.rx_channel_number)
            iq_data_dict[iq_data.rx_channel_number] = iq_data
    alignment = calculate_ptp_alignment_all(iq_data_dict, args.base_rx)

    for data in alignment:
        plot_alignment(iq_data_dict[data], iq_data_dict[args.base_rx], alignment)
    
if __name__ == "__main__":
    sys.exit(not main())