"""
IQ Data Class
"""

from fileinput import filename
import os
import numpy as np
import scipy
from abc import ABCMeta, abstractmethod

class IQdat(object):
    
    def __init__(self,filename) -> None:
        self.filename = filename
        self.folder = ''
        self.fs = 0.0
        self.nsamples = 0
        self.data_array = None
        self.filesize = 0
        self.format = ''

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

    @abstractmethod
    def read_samples(self, nsamples, offset):
        pass