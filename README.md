# Cognitive Radar Testbed Reference Architecture

Welcome to the Cognitive Radar Testbed Reference Architecture software distribution. refarch-multich is the free and open-source reference architecture code for building a multi-channel USRP based Cognitive Radar Testbed, created and sold by NI.

refarch-multi supports n320 and n321 Ettus USRP hardware.

## Documentation

For technical documentation related to USRP™ hardware or UHD system design, check out the [UHD and USRP Manual](http://files.ettus.com/manual/). That is where you can find [Installation Instructions](http://files.ettus.com/manual/page_install.html), help on how to [build UHD from source](http://files.ettus.com/manual/page_build_guide.html) on different platforms, development guidelines and reference documentation as well as device usage guidance.

Additionally, be sure to check out the Ettus Research [FAQ](https://kb.ettus.com/Technical_FAQ), and the [Knowledge Base](http://kb.ettus.com/) for useful application notes and tutorials.

## OS Support

refarch-multich is primarily developed and tested on Linux Ubuntu 20.04 version. Other operating systems will most likely work, too, but are not officially supported.

## Applications

rearch-multich can be used to build stand-alone Cognitive Radar testbeds with USRP™ hardware, or with third-party applications. Some common use-cases for the framework are

- 

## Directories

**host/**

The source code for the user-space driver.

**tools/**

Additional tools, mainly for debugging purposes. See the readme-file in that directory for more details on the individual tools.

## Instructions

1. Install latest version of Python3 virtual environment using the command `apt-get install python3-venv`

2. Download setup_script.sh to Home directory
3. Make the script executable using the command `chmod +x setup_script.sh`
4. Run the script using the command `./setup_script.sh`