import unittest
import Arch_analysis
import argparse
import glob
import os
import shutil
import numpy as np

class Archtest_tests(unittest.TestCase):

    def setUp(self):
        self.data_dict = {}
        directory = None
        if directory is None:
            folders = glob.glob("/mnt/md1/" +"*")
            fileFolder = max(folders, key=os.path.getctime)
        print(fileFolder)
        for file in os.listdir("/"+fileFolder):
            if file.endswith(".dat"):
                data_array = np.fromfile(fileFolder+"/"+file, dtype=np.int16)
                rx_channel = Arch_analysis.extract_channel_rx(file)
                self.data_dict[rx_channel] = data_array
    
    def test_extract_channel_rx_00(self):
        actual = Arch_analysis.extract_channel_rx("123123_134adf_rx_00_tx_01")
        expected= "rx_00"
        self.assertEqual(actual, expected)

    def test_extract_channel_rx_16(self):
        actual = Arch_analysis.extract_channel_rx("123123_134adf_rx_16_tx_01")
        expected= "rx_16"
        self.assertEqual(actual, expected)

    def test_extract_channel_tx_01(self):
        actual = Arch_analysis.extract_channel_tx("123123_134adf_rx_00_tx_01")
        expected= "tx_01"
        self.assertEqual(actual, expected)
    
    def test_extract_channel_tx_15(self):
        actual = Arch_analysis.extract_channel_tx("123123_134adf_rx_00_tx_15")
        expected= "tx_15"
        self.assertEqual(actual, expected)

    def test_phase_align_self(self):
        actual = Arch_analysis.measure_phase(self.data_dict["rx_00"], self.data_dict["rx_00"])
        expected = [0] * len(self.data_dict["rx_00"])
        np.allclose(actual, expected)

    