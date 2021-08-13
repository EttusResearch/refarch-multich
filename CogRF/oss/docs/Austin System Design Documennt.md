# Austin System Design Document

[[_TOC_]]

## Problem Statement
<!-- 
   Brief description of the feature and the work involved to implement it.
-->

Austin Release is focused on the Open Source Software (OSS) version of CogRF testbed for algorithm exploration. Features in this release aim to address some of the industry trends related to reducing time to market for developing new techniques/architectures that deliver improved radar/EW system performance, migration towards fully-digital active phased arrays, and heterogeneous system integration of CPU/GPU with FPGA for cognitive systems. Austin is focused on addressing the challenge that OSS customers face in taking concepts from the whiteboard and implement them in hardware. Their needs are to minimize time spent in architecting/developing the hardware test bed and minimize the time spent in porting algorithms from software to hardware. Austin is also focused on addressing the challenges that OSS customers face in iterating on algorithms to maximize performance. Their needs are an integrated workflow that helps them validate system performance of new algorithms/architectures in real-world scenarios, leveraging EM simulation in the lab all the way to field trials. Customer base includes top target accounts such as DoD Research Labs (ARL, AFRL, NRL), FFRDCs (MIT Lincoln Lab, MITRE), Contractors (Raytheon, LM, NGC). Top competitors in this space include Pentek, Abaco, Annapolis Microsystems, Curtiss Wright, and Do It Yourself (DIY) customer base.

To address these stakeholder needs, the key system requirements are

1. Deliver a system infrastructure to provide a higher-level starting point for addressing multi-channel synchronization and data streaming for USRP.
2. Deliver a system infrastructure to provide a higher-level starting point for addressing multi-channel synchronization and data streaming for PXI-based systems using FlexRIO.
3. Deliver software improvements for better interoperability with system design workflows.
4. Design new products specifically targeted for Radar/EW, including SDRs, FPGA boards, CPU/GPU integration.
5. Deliver improved technical consulting capability.

Austin Release is mainly focused on (1) and (3).

## Links To Relevant Work Items and Reference Material
<!-- 
   Include links to the work items this design addresses.
   Include links to any other meaningful reference material.
-->



![Design](.\Design.png)

Figure 1: Shows 32x32 NI USRP-based CogRF test bed architecture for the Austin Release

Goal of Austin release is to help customers quickly scale from software algorithm design to hardware demonstration with NI's customizable, COTS test bed for radar and electronic warfare technique development. To achieve this goal, Austin will deliver 

•  An NI-USRP based testbed (n320/n321 based) Direct Conversion RF Sampling (200 MHz IBW) in the L, S, C frequency bands.

•  Phase coherent operation through duration of experiments w/ LO, Reference Clock, and PPS sharing.

•  Documentation and example code for multi-channel synchronization and waveform generation/acquisition.

•  Documentation for interoperability between GPU-based DSP and UHD driver.

•  Documentation for interoperability between MATLAB and UHD driver.

•  Documentation showing key system level specifications for Aggregate Max Data Transfer Rate (GB/s) to/from PC Memory, Aggregate Max Data Transfer Rate (GB/s) to/from Disk, Stability of Phase Coherence during an experimentation run (32-channel, Tx-Tx, Rx-Rx, Tx-Rx) configuration. Note that achieving this specification will require that the customer perform system-level phase alignment operation (incl. custom front-end) on system power up to de-skew phase at the operating carrier frequency. Also note that the streaming performance is dependent on server configuration.

Links to feature specification documents for key features are provided below

C++ Reference Code that demonstrates synchronized streaming example



C++ Reference Code that demonstrates how to achieve Tx-Tx, Rx-Rx, Tx-Rx phase stability across 32-channels (4x4, 16x16 will also be demonstrated as part of the 32x32 setup) <add link to feature spec>

C++ Reference Code that demonstrates how to achieve streaming from  32-channels (4x4, 16x16 will also be demonstrated as part of the 32x32 setup) <add link to feature spec>

Example that demonstrates how to use UHD drivers and Matlab for Dopper phase shift example and Machine Learning example

Example that demonstrates how to use UHD drivers and GPU for Machine Learning example

Documentation on recommended server configuration to support the published specifications. 

**Nice-to-have**

Python Reference Code that demonstrates how to achieve Tx-Tx, Rx-Rx, Tx-Rx phase stability across 32-channels (4x4, 16x16 will also be demonstrated as part of the 32x32 setup) <add link to feature spec>

Python Reference Code that demonstrates how to achieve streaming from  32-channels (4x4, 16x16 will also be demonstrated as part of the 32x32 setup) <add link to feature spec>

**Installation Details**



1. Baseline experience: Host source code on https://github.com/EttusResearch as CogRF-OSS (name to be decided). Readme (something similar to https://files.ettus.com/manual/page_build_guide.html) provides step by step instruction. Includes source code and documentation

2. Next level, create installation packages (debian) and we would host them on a NI-managed server. Discuss with Ettus team about preferred user experience
3. Documentation only (readme, spec sheet, how tos) duplicated on files.ettus.com

## Implementation / Design
<!-- 
   Describe the implementation and what systems will be affected. Items to consider include:
   * Does this design follow an existing design in the code
   * Does this design create new requirements on clients
   * Does the design have any performance considerations
   * What are unique aspects of this design and how are they going to be handled
   * Are there UX considerations that need to be accounted for
-->

Support for N320/N321​

USRP configuration (32-channel system)
Documentation on how to scale the reference architecture for higher channel count systems

Infrastructure: Manages Out of the box experience for the customer. Seamlessly handle data streaming (to and from PC memory), data streaming (to and from disk), multi-channel phase-coherent RF

Infrastructure should be such that it is understandable and modifiable by the customer

High level examples that show customers how to use the reference architecture

Documentation 

Phase coherent RF (TClk-like performance is sufficient, hooks for array cal by customer)​

10/40 GbE data movement to server w/ CPU/GPU, guidance on data movement to GPU​
streaming to disk (how many channels, what bandwidth)

Reference architecture getting started example w/ multichannel synchronized RF to FPGA processor to server​

IP (Host only)​

Basic waveform generation from UHD​ (load wavefrom file, specify generation parameters such as repetition frequency
, seed, number of samples, ....)

Ability to play out at deterministic pulse repetition frequencies

Support for both buffered acquisition and streaming applications. need to have a target in mind
continuous streaming for "n" channels with "y"

Basic receive acquisition engine, finite, in-order records streamed to coprocessor​

### Public API changes
<!--
   Describe expected changes to the public API.
	Will any existing APIs be deprecated?
	Will new public APIs be introduced into the core product?
-->

Austin does not add any new public API

## Alternative Implementations / Designs
<!-- 
   Describe any design alternatives and discuss why they were rejected.
-->

## Open Issues
<!-- 
   Describe any open issues with the design that you need feedback on before proceeding.  It is expected that these issues will be resolved during the review process and this section will be removed when this documented in pulled into source.
-->

[Link]: https://ni.visualstudio.com/DevCentral/_wiki/wikis/AppCentral.wiki/15078/Project-Documentation

 to open issues document

