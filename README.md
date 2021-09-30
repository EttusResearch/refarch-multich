# Open Architecture for Radar and EW Research

Welcome to the Open Architecture for Radar and EW Research software distribution. refarch-multich is the free and open-source reference architecture code for building a multi-channel USRP based Radar and EW Research Testbed, created by NI.

refarch-multich supports Ubuntu 20.04 with the N320 and N321 Ettus USRP hardware.

## Documentation

For getting started assistance, check out the User Manual located at the [Knowledge Base](http://kb.ettus.com/). For technical documentation related to USRP™ hardware or UHD system design, check out the [UHD and USRP Manual](http://files.ettus.com/manual/).

## OS Support

refarch-multich is developed and tested on Linux Ubuntu 20.04 version. We have not tested and are not officially supporting other operating systems or versions but if UHD is able to be built on your system this architecture will likely work.

## Applications

rearch-multich can be used to build stand-alone Radar and EW Research testbeds with USRP™ hardware, or with third-party applications. Some common use-cases for the framework are

- Cognative Radar Testbed
- Antenna Array
- Beamforming Algorithms 
- Waveform Testing

## Directories

**config/**
Utilities to build a Radar and EW Research testbed. Includes Ring Buffer script, and example Yaml file.

**lib/**
The functions used to setup, configure, test, and use the Testbed.

**examples/**
The example application and configuration file.

**tools/**
Utilities to generate and test DAT files that are used in the example application.


## Instructions

Read the Open Architecture for Radar and EW Research User Manual located on the Ettus KB website. 

**quick start**
1. Clone or download this repository to the directory of your choice.
2. Open a terminal in the root folder.
3. Make the setup script executable using the command `sudo chmod +x setup_script.sh`
4. Run the script using the command `sudo ./setup_script.sh`

