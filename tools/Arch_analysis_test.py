import unittest
import Arch_analysis

class Archtest_tests(unittest.TestCase):
    
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