import datetime
import os
import struct
import logging as log
import numpy as np
import re
import sys

from usrpDat import *
from util import *

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
        
    def get_window(self, n=None):
        if not n:
            n = self.framelength
        assert self.window in ['rectangular',
                               'bartlett', 'blackman', 'hamming', 'hanning']
        if self.window == 'rectangular':
            return np.ones(n)
        elif self.window == 'bartlett':
            return np.bartlett(n)
        elif self.window == 'blackman':
            return np.blackman(n)
        elif self.window == 'hamming':
            return np.hamming(n)
        else:
            return np.hanning(n)

    def fft_freqs(self):
        n = len(self.complex_data)
        ts = 1.0 / self.fs
        f = np.fft.fftfreq(n, ts)
        return np.fft.fftshift(f)

    def get_fft(self, x=None, n=0, l=0):
    
        if n and l:
            nf = n
            lf = l
        else:
            nf = 1
            lf = len(self.complex_data)
           
            if x is not None:
                lf = len(x)

        if x is None:
            data = self.complex_data
        else:
            data = x

        termination = 50  
        data = np.reshape(data, (nf, lf))
        freqs = self.fft_freqs()
        v_peak_iq = np.fft.fft(
            data * self.get_window(lf), axis=1)
        v_peak_iq = np.average(v_peak_iq, axis=0) / lf * nf
        v_rms = abs(v_peak_iq) / np.sqrt(2)
        p_avg = v_rms ** 2 / termination
        
        return freqs, np.fft.fftshift(p_avg), np.fft.fftshift(v_peak_iq)

    def get_dbm(watt):
        
        if isinstance(watt, np.ndarray):
            watt[watt <= 0] = 10 ** -30
        return 10 * np.log10(np.array(watt) * 1000)