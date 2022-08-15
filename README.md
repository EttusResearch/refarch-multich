# Multichannel RF Reference Architecture

Welcome to the Multichannel RF Reference Architecture software distribution. refarch-multich is the free and open-source reference architecture code for building a Multichannel RF Reference Architecture system, designed by NI.

refarch-multich supports Ubuntu 20.04 with the USRP N320 and USRP N321 Ettus hardware.

## Documentation

For getting started assistance, visit the User Manual located at the [Multichannel RF Reference Architecture](https://kb.ettus.com/Multichannel_RF_Reference_Architecture). For technical documentation related to USRP™ hardware or UHD system design, visit the [UHD and USRP Manual](https://files.ettus.com/manual/).

## OS Support

refarch-multich is developed and tested on Ubuntu 20.04 version. Operating systems and versions not listed in this document have not been tested and are not officially supported.

## Applications

rearch-multich can be used to build stand-alone Multichannel RF Reference Architecture system with USRP hardware, or with third-party applications. Some common use-cases for the framework are as follows:

- Cognitive Radar
- Antenna Array
- Beamforming Algorithms 
- Waveform Testing

## Directories

**config/**
Utilities to setup and configure the server and USRPs. Includes Ring Buffer script, and example .yaml file.

**lib/**
The functions used to setup, configure, test, and use the system.

**examples/**
The example application and configuration file.

**tools/**
Utilities to generate and test .dat files that are used in the example application.

**docs/**
Documents including the BOM, wiring list, and templates to setup the system.


## Instructions

Read the Multichannel RF Reference Architecture User Manual located on the [Ettus KB website](https://kb.ettus.com/Multichannel_RF_Reference_Architecture). 

**Setup Script**
1. Clone or download this repository to the directory of your choice.
2. Open a terminal in the root folder.
3. Make the setup script executable using the command `sudo chmod +x setup_script.sh`
4. Run the script using the command `sudo ./setup_script.sh`

