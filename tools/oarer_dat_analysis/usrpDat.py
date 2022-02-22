import datetime
import os
import struct
import logging as log
import numpy as np
import re
import sys

from usrpDat import *
from util import *
from tone_measurement_module import ToneMeasurement
class usrpDat(object):

    def __init__(self, filename, datatype, filesize, args) -> None:
        
        self.filename = filename
        self.datatype = datatype
        self.i = []
        self.q = []
        self.complex_data = []
        self.folder = ''
        self.fs = 0.0
        self.nsamples = 0
        self.data_array = None
        self.filesize = filesize
        self.rx_channel_number = self.extract_channel_rx()
        self.tx_channel_number = self.extract_channel_tx()
        self.framelength = args.l
        self.nframes = args.n 
        self.startframe = args.s
        self.window = 'rectangular'
        self.method = 'fft'
        
    def __str__(self):
        return self.dic2htmlstring(vars(self))

    def dic2htmlstring(self, dic):
        outstr = ''
        if 'filename' in dic:
            outstr += '<font size="4" color="green">File name:</font> {} <font size="4" color="green"></font><br>\n'.format(
                self.filename)
        if 'fs' in dic:
            outstr += '<font size="4" color="green">Sampling rate:</font> {} <font size="4" color="green">[sps]</font><br>\n'.format(
                self.fs)
        if 'nsamples' in dic:
            outstr += '<font size="4" color="green">No. Samples:</font> {} <br>\n'.format(
                self.nsamples)
        return outstr

    def read_samples(self, datatype):
        self.data_array = np.fromfile(self.folder + "/" + self.filename, dtype=datatype)
    

    def extract_channel_rx(self):
        """Look in filename for rx channel"""
        match_rx= re.search("rx_[0-9][0-9]", self.filename)
        self.rx_channel_number = match_rx.group()
        return self.rx_channel_number

    def extract_channel_tx(self):
        """Look in filename for tx channel"""
        match_tx= re.search("tx_[0-9][0-9]", self.filename)
        self.tx_channel_number = match_tx.group() 
        return self.tx_channel_number

    def deinterleave_iq(self):
        """Deinterleave a 1D array into two 1D arrays [0,1,2,3]>>>[[0,2],[1,3]]"""
        self.i, self.q = [self.data_array[index::2] for index in range(2)]
        return self.i, self.q

    def determine_nsamps(self):
        if (self.datatype == "np.int16"):
            self.nsamps = self.filesize/4
        else:
           print("Unknown data format, not yet supported..")
           sys.exit() 

    def convert_to_complex(self):
        complex_data = []
        for x in range(0,len(self.i)):
            self.complex_data.append(np.complex(self.q[x], self.i[x]))
        
    