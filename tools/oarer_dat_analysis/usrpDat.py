
#
# Copyright 2010-2012,2014-2015 Ettus Research LLC
# Copyright 2021 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

import logging as log
import numpy as np
import re
import sys
from usrpDat import *
from util import *
class usrpDat(object):

    def __init__(self, filename, datatype, filesize) -> None:
        
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
        self.data_array_full = None
        #TODO
        self.ptpskew = None
       
    def read_samples(self, datatype, offset):
        temp = np.fromfile(self.folder + "/" + self.filename, dtype=datatype)
        self.data_array_full = temp 
        self.data_array = temp[offset*2:]
        
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
        if (self.datatype == "int16"):
            self.nsamples = self.filesize/4
        else:
           print("Unknown data format, not yet supported..")
           sys.exit() 

    def convert_to_complex(self):
        complex_data = []
        for x in range(0,len(self.i)):
            self.complex_data.append(np.complex(self.q[x], self.i[x]))
        
    