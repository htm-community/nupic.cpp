# NetworkAPI Documentation

Network API is all about experimenting and building apps from the HTM building blocks (or regions) which are wrappers around the htm algorithms which implement the HTM theories developed by Jeff Hawkins and the team at [Numenta](https://numenta.com/)

This document describes the NetworkAPI component which is part of the htm.core library that is being maintained by the [HTM Community](https://numenta.org).  The open source code and installation instructions are available on [GitHub at github.com/htm-community/htm.core](https://github.com/htm-community/htm.core).

NetworkAPI is written as a C++ library with Python 3.x bindings. It is expected that there will be a user written app that directs the placement of the building blocks and data flows that make up an application or experiment.  That app can be written in C++ or Python 3.x and linked directly to the htm.core library. These are supported on Linux, OSx, and Windows platforms.  The app can also interface to the htm.core library via a REST web interface.  

The intent of NetworkAPI is to strip away the complexity of working directly with the HTM algorithms so that users can concentrate on discovering and refining the concepts that underline Intellegence.

## NetworkAPI components
- [NetworkAPI Engine](NetworkAPI_Engine.md); The Network class and how it runs. (doc coming soon)
- [NetworkAPI Built-in Regions](NetworkAPI_Regions.md); The predefined building blocks
- [NetworkAPI Links](NetworkAPI_Links.md); How data flows between blocks
- [NetworkAPI REST interface](NetworkAPI_REST.md); A Web interface
- [NetworkAPI How to create Regions](NetworkAPI_Creating_Regions.md); Making custom building blocks.

## Example

There is simple python example of 'Hotgym experiment' located in /py/htm/examples/networkAPI folder.

## Help
In addition to the [Peer Reviewed Reasearch Papers](https://numenta.com/neuroscience-research/research-publications/papers/) released by Numenta, there is Matt Taylor's [HTM School](https://numenta.org/htm-school/) which explains the concepts and an active [User Forum](https://discourse.numenta.org/categories) where you can ask for help.

NetworkAPI is installed as part of the htm.core library. For installation instructions refer to the [README file in the htm.core repository](https://github.com/htm-community/htm.core). 

Bugs a feature requests can be submitted to the [Github Issues list](https://github.com/htm-community/htm.core/issues) on the htm.core repository.
