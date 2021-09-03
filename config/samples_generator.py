import math 
import struct
import argparse
import matplotlib.pyplot as plt
import cmath
import sys

WAVE_TABLE_LENGTH = 8192


def parse_args():
    """Parse the command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument("-f", "--file-path", default="test.dat", type=str)
    parser.add_argument("-s", "--sample-rate", default=250000000, type=int)  
    parser.add_argument("-a", "--amplitude", default=.3, type=int)
    parser.add_argument("-freq","--frequency", default = 500000, type=int)
    parser.add_argument("-txrate","--tx_rate", default = 250000000, type = int)
    parser.add_argument("-method","--method",default = "wavetable", type = str)
    parser.add_argument("-nsamples","-nsamples", default = 16000, type = int)
    return parser.parse_args()



def main():
    args = parse_args()
    #This method is an alternate way to generate data but is not
    #guaranteed to work with the replay block. 
    if (args.method == "alternate"):
        
        file = open(args.file_path, "wb")

        for i in range(args.nsamples):
            I = int((2**15-1) * args.amplitude * math.cos(i / (args.sample_rate / args.frequency) * 2 * math.pi))
            Q = int((2**15-1) * args.amplitude * math.sin(i / (args.sample_rate / args.frequency) * 2 * math.pi))
            # <2h formatting indicates little endian, short format for both I and Q values. The 2 specifies that the 2 values (I and Q) each have the format.
            file.write(struct.pack('<2h', I, Q))
   
        
        file.close()

    else: 
        #This method replicates the wavetable.hpp method provided in UHD 4.0
        wave_table = []

        #error when wavefrom is not possible to generate
        if (abs(args.frequency) > args.tx_rate/2):
            sys.exit("ERROR: Wave Freq out of Nyquist Zone")
        if (args.tx_rate/abs(args.frequency) > WAVE_TABLE_LENGTH/2):
            sys.exit("ERROR: Wave Freq Too Small for Table")
        step = round(args.frequency/args.tx_rate * WAVE_TABLE_LENGTH)
        
        f = open(args.file_path, "wb")

        tau = 2 * math.acos(-1.0)
        J = complex(0,1)
        index = 0
        #print(J.imag)
        for i in range(WAVE_TABLE_LENGTH):
            
            
            wave_table.append(args.amplitude * cmath.exp((tau*i/WAVE_TABLE_LENGTH)*J))
        
    
            
        
        while (index < len(wave_table)):
            I = int((2**15-1)*wave_table[index].real)
            Q = int((2**15-1)*wave_table[index].imag)
            
            # <2h formatting indicates little endian, short format for both I and Q values. The 2 specifies that the 2 values (I and Q) each have the format. 
            f.write(struct.pack('<2h',I, Q))
            index += step

    




        f.close()


if __name__ == "__main__":
    main()

