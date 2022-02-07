import datetime
import os
import struct
import logging as log
import numpy as np
import re
import sys

from IQdata import IQdat
from util import *

class usrpDat(IQdat):

    def __init__(self, filename, datatype, filesize) -> None:
        super().__init__(filename)
        self.rx_channel_number = self.extract_channel_rx()
        self.tx_channel_number = self.extract_channel_tx()
        self.i = []
        self.q = []
        self.filename = filename
        self.datatype = datatype
        self.filesize = filesize

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
        array=[]
        self.i, self.q = [array[index::2] for index in range(2)]
        return self.i, self.q

    def determine_nsamps(self):
        if (self.datatype == "np.int16"):
            self.nsamps = self.filesize/4
        else:
           print("Unknown data format, not yet supported..")
           sys.exit() 

