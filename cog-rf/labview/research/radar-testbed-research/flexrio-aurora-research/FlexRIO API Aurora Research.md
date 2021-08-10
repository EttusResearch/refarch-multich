# FlexRIO API Aurora Research

[[_TOC_]]

## Examples

### Forum Example from Cason Longley
There is only one example for Aurora that uses the FlexRIO API. It uses the 10.4167 Gbps line rate that is used for the 5840 VST. 
- https://forums.ni.com/t5/Examples-and-IP-for-Software/Aurora-64b-66b-Streaming-Example-for-the-PXIe-7915-Ultrascale/ta-p/3952187?profile.language=en
- The CLIP from this example can be ported into any other NI Ultra Scale module that has the nanopitch connector. 
- However, if another CLIP is needed then a new Aurora core will have to be Generated

### SlicGen
Andrew Moch, Chief Digital Hardware Engineer has created a tool that creates Aurora cores with user configurable options. There is a Teams team called "MGT as 1st Class Citizen" that has more information. Andrew would like to get by in for this tool from others and is willing to provide support. The tool supports various transcievers as well. GTX, GTH, and GTY.

SlicGen (Socked Labview Ip Clip Gerator) is designed to take some of the heavy lifting out of using HighSpeed Serial IO.   
- SlicGen Flow 
    - (1) Core Test Script (modified by SE) -> Vivado -> Socketed Clip  
    - (2) Socketed Clip-> LV scripting -> Example Project  
    - (3) Example Proejct ->  LvFPGA -> Bit-file 
    - (4) Bit-file + Host App.vi -> LabVIEW -> Working Serdes Example 

### Future Examples
The 6593 (high speed serial device) has a nanopitch conenctor and will come with shipping examples for various Aurora line rates (from my understanding). Since it too uses an Ultra Scale the clip from those examples could be imported to other Integrated IO FlexRIOs





