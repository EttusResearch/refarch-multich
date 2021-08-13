# Multi-Channel Tx/Rx Synchronization

[[_TOC_]]

## Who
<!-- 
	Provide contact information.
-->

_Author:_ Abhay Samant, David Asplund

_BusinessUnit:_ ADG

_Team_ <team>

## Feature Work Item
<!-- 
   Link to the Feature work item which should contain a Description (Problem Statement) and Acceptance Criteria.
   If this document covers multiple features, then provide a link to all work items.
-->

- [####: Feature Name](http://link_to_feature)
- [####: Feature Name](http://link_to_feature)

## Links To Reference Material
<!-- 
   Include links to any other meaningful reference material such as:
   * UX specifications
   * Usability studies
   * Other architecture/design documents
   * Design meeting minutes
-->

## Implementation / Design
<!-- 
   Describe the implementation and what systems will be affected. Items to consider include:
   * Does this design follow an existing design in the code?
   * Does this design create new requirements on clients?
   * Does the design have any performance considerations?
   * What are unique aspects of this design and how are they going to be handled?
   * Are there UX considerations that need to be accounted for?
-->

A key component required for validating this feature is figuring out how to retreive the values saved by the 
rx_samples_to_file example. This example allows user to specify the data type for the samples saved to the rx_samples_to_file
<br/>--type arg (=short)            sample type: double, float, or short


LabVIEW has a built-in function as shown below
 



## Alternative Implementations / Designs
<!-- 
   Describe any design alternatives and discuss why they were rejected.
-->

## Open Issues
<!-- 
   Describe any open issues with the design that you need feedback on before proceeding. 
   It is expected that these issues will be resolved during the review process and this section will be removed when this documented in pulled into source.
-->

## Product Inclusion
<!-- 
   Document how this feature is included into products
   Is it an Add-on that can be installed separately
   What feature toggles does the feature use
   Is there an opt-in / out that products can use 
-->

### Feature Toggles
<!--
   List the feature toggles which control the availability of this feature
-->

### Opt-in / Out
<!--
   List the APIs products can use to opt-in / out of the feature
-->

### Public API changes
<!--
   Describe expected changes to the public API.
	Will any existing APIs be deprecated?
	Will new public APIs be introduced into the core product?
-->

## Impact Considerations
<!--
	The purpose of this section is to ensure considerations are made for areas of the product, platform or ecosystem that may not be obvious in the development of your feature.
	If a row applies to the feature, add documentation, contact names, or links to reference material on the impacted area. 
	The list below includes areas that could be affected at the time but it may not be a complete list - add items as needed. 
	Filling out this section is not a sufficient means to notify the impacted team, you will need to consult with the impacted team! 
	You can begin by reaching out to the Key Contact listed in the Comments of the entry.	
-->

**Content / Documentation**
<!-- 
	In general, the content required by your feature will be identified by the content council for your product. 
	If your feature needs documentation, example programs, tutorials, or references beyond what is typical, call that out here and/or link to resources that provide more details. 
	Key contact: <lindsey.olson@ni.com>
-->

**Installer / Package Manager**
<!-- 
	Include any unusual installer or package manager implications the feature introduces. 
	Key contacts: <shawn.ebert@ni.com> (installer) and <laRisa.Mohr@ni.com> (Package Manager).
-->

**Localization**
<!-- 
	Does the feature require any unusual localization needs?  No need to document standard localization requirements.
	Key contacts: <steve.orth@ni.com> (internationalization) and <michel.farhi-chevillard@ni.com> (localization)
-->

**NI Licensing**
<!-- 
	Include information on any special NI License manager implications for the feature (e.g. any new feature/licensing situations). 
	Refer to [Licensing FAQ](https://nitalk.jiveon.com/docs/DOC-401330) for more information. 
	Key contact: <devchandar.mohan@ni.com>.
-->

**Third Party Licensing**
<!-- 
	If you are adding or updating any 3rd-party components, list them here and follow this process to insure we license it properly: https://nitalk.jiveon.com/docs/DOC-428813
	Key contact: <clint.fletcher@ni.com>
-->

**CEIP**
<!-- 
	Work with your Project Manager and Marketing contact to identify any CEIP data that you want to gather on feature usage. You can link to an external document/discussion. 
	Key contact: <marc.page@ni.com>
-->

**Centralized Test Team (CTT)**
<!-- 
	Include any asks for CTT resources or implications. 
	Key contact: <praveen.madabushi@ni.com>.
-->

**IT**
<!-- 
	If your feature requires work from IT, include that here.
-->
 
**Other R&D team (e.g. a driver or another squad)**
<!-- 
	If you are requesting resources for any team other than the feature team listed, note that here. 
	For example, if your feature requires driver teams to make changes, you would note that here.
-->
