# High Level Design [Template]

[[_TOC_]]

## Problem Statement
<!-- 
   This document contains high level design description of the Cognitive Radar Testbed System Offering. This document will describe,
   at a high level, all the features in this offering. Version specific features will be captured in individual design documents.
   
-->
This document contains high level design description of the Cognitive Radar Testbed System Offering. This document will describe, 
at a high level, all the features in this offering. Version specific features will be captured in individual design documents. Features
included in the initial release include, ideal release around Q4, 2020


LV Toolchain​

Support for 5785 (2x2 3.2G transceiver), 1095, ATCA, 7902​

Infrastructure​

Phase coherent RF (TClk good enough, hooks for array cal by customer)​

Aurora data movement to FPGA coprocessor​

10/40 GbE data movement to server w/ CPU/GPU, guidance on inter-process data movement to GPU​

Reference architecture Getting Started example w/ multichannel synchronized RF to FPGA processor to server, 
incl. basic UI for viewing ​

Stream to disk of raw IQ or processed data from server​

IP​

Basic waveform generation (triggered playback from FPGA BRAM)​

Basic receive acquisition engine, finite, in-order records streamed to coprocessor​

Toolchain​

Documented paths from MW tools to test bed via HDL Coder​
Timing and Triggering

Open Source Toolchain​

Support for N320/1, ATCA​
Single USRP configuration
 USRP configuration (16-channel system)
 Documentation on how to scale the reference architecture for higher channel count systems

Infrastructure​: Manages Out of the box experience for the customer. Seamlessly handle data streaming (to anf from host),
data streaming (host-fpga), multi-channel phase-coherent RF, sandbox approach to algorithm development with couple of 
examples of text book level implementations of algorithms already implemented.

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

support for both buffered acquistion and streaming applications. need to have a target in mind
continuous streaming for "n" channels with "y"

Basic receive acquisition engine, finite, in-order records streamed to coprocessor​

## Links To Relevant Work Items and Reference Material
<!-- 
   Include links to the work items this design addresses.
   Include links to any other meaningful reference material.
-->

## Implementation / Design
<!-- 
   Describe the implementation and what systems will be affected. Items to consider include:
   * Does this design follow an existing design in the code
   * Does this design create new requirements on clients
   * Does the design have any performance considerations
   * What are unique aspects of this design and how are they going to be handled
   * Are there UX considerations that need to be accounted for
-->

### Public API changes
<!--
   Describe expected changes to the public API.
	Will any existing APIs be deprecated?
	Will new public APIs be introduced into the core product?
-->

## Alternative Implementations / Designs
<!-- 
   Describe any design alternatives and discuss why they were rejected.
-->

## Open Issues
<!-- 
   Describe any open issues with the design that you need feedback on before proceeding.  It is expected that these issues will be resolved during the review process and this section will be removed when this documented in pulled into source.
-->
