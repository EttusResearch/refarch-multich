# Open Architecture for Radar and EW Research

Welcome to the Open Architecture for Radar and EW Research software distribution. refarch-multich is the free and open-source reference architecture code for building a multi-channel USRP based Radar and EW Research Testbed, created by NI.

refarch-multi supports Ubuntu 20.04 with the N320 and N321 Ettus USRP hardware.

## Documentation

For technical documentation related to USRP™ hardware or UHD system design, check out the [UHD and USRP Manual](http://files.ettus.com/manual/). That is where you can find [Installation Instructions](http://files.ettus.com/manual/page_install.html), help on how to [build UHD from source](http://files.ettus.com/manual/page_build_guide.html) on different platforms, development guidelines and reference documentation as well as device usage guidance.

Additionally, be sure to check out the Ettus Research [FAQ](https://kb.ettus.com/Technical_FAQ), and the [Knowledge Base](http://kb.ettus.com/) for useful application notes and tutorials.

## OS Support

refarch-multich is primarily developed and tested on Linux Ubuntu 20.04 version. Other operating systems will most likely work, too, but have not been tested and are not officially supported.

## Applications

rearch-multich can be used to build stand-alone Radar and EW Research testbeds with USRP™ hardware, or with third-party applications. Some common use-cases for the framework are

- 

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

