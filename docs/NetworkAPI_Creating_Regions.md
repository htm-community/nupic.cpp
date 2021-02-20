# How to create C++ Regions for NetworkAPI

This doc is for those that would like to create a new Region using C++.  

## Background
A Region for NetowrkAPI is a wrapper around an algorithm which provides a consistent/uniform interface to the engine that executes the regions.  The objective is to allow a new algorithm to be added to the NetworkAPI tool set with the minimum of effort.

The Region is implemented as a plugin.  This means that they could be dynamically loaded (i.e. registered) and executed without having to be linked into the htm.core library.  The C++ Built-in regions are a special set of regions that are plug-ins in their own right but they are linked into the htm.core library and pre-registered to make them easier to use.

## The Region API
The NetworkAPI engine, represented by the Network class, will execute each of the configured regions on each iteration of a run. Each configured region is presented to the Network class as the generic Region class. This class in turn acts as the interface to the actual implantation of that region in a plug-in. Data will flow between the plug-in regions according to the links that have been configured. It is this plug-in that we need to implement to create a new Region for the NetworkAPI.

In order to be used with the NetworkAPI Engine as a plug-in, a C++ region implementation must:
- must provide a static object `Spec` which describes the parameters used by the plug-in and its inputs and outputs.
- be a subclass of RegionImpl which can be found in `src/htm/engine/RegionImpl.hpp` and a subclass of Serializable.  
- must accept a reference to the generic Region class instance in the constructor and pass it to the subclass.
- must accept pre-parsed configuration parameters in the constructor.
- must implement a second constructor for use by Cereal serialization.
- may implement getters and setters for parameters that can be accessed or set after the region has been instantiated.
- may implement an initialization function that is called during the initialization phase of the engine.
- must implement the function `askImplForOutputDimensions(name)` if returning full dimensions for outputs or implement `getNodeOutputElementCount(name)` if returning only the one dimensional size of each output.
- must implement the `compute( )` method which gets called by the engine during a run iteration.
- must implement save_ar( ... ) and load_ar( ... ) to implement serialization for the plug-in.

In addition, if the new region is to be a built-in plugin, it needs to be included in the list of region implementation classes in the factory that are registered when the engine starts up. The list is at about line 92 of `src/htm/engine/RegionImplFactory.cpp`.

The NetworkAPI engine and all of its generic components are found in `src/htm/engine`.  The build-in C++ region implementations are placed in `src/htm/regions`. The core algorithms and encoders have their own folder.  General utilities for algorithms are found in `src/htm/utils` and `src/htm/types`.  Utilities for NetworkAPI are placed in `src/htm/ntypes`.

The algorithms and encoders should not access any class or function in NetworkAPI or its utilities.  The NetworkAPI routines should restrict access to the algorithms routines to only the region implementations which wrapper them.

## Implementation Details

We will use the RDSEEncoderRegion class as an example.  See `src/htm/regions/RDSEEncoderRegion.hpp`. 

### copyright
First you need a copyright notice if this is to be included in the htm.core library. There should be one of these in both the .hpp and .cpp source files.
```
/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2020, Numenta, Inc.
 *
 * Author: <your name>, <date>, <your email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 * --------------------------------------------------------------------- */
 ```
 
### constructors
Then we need to create a class for the new region implementation. For our example we are using RDSEEncoderRegion.  By convention, the name of the region class is the name of the algorithm followed by "Region".  The region's type name can be different when you register it but it is normally the same.
So your .hpp file you need something like this:

```
class RDSEEncoderRegion : public RegionImpl, Serializable {
public:
  RDSEEncoderRegion(const ValueMap &params, Region *region);
  RDSEEncoderRegion(ArWrapper &wrapper, Region *region);

  virtual ~RDSEEncoderRegion() override;
```

You can see we have our class name and it is a sub-class of RegionImpl and Serialzable.  We have the two constructors, one for initial creation and one to use when restoring a serialization.  The destructor is optional.  All you need here is to replace our example class name with your new class name.

Over in the .cpp you need the two constructor implementations.
```
RDSEEncoderRegion::RDSEEncoderRegion(const ValueMap &par, Region *region) : RegionImpl(region) {
  rnd_ = Random(42);
  spec_.reset(createSpec());
  ValueMap params = ValidateParameters(par, spec_.get());
    
  RDSE_Parameters args;
  args.size =       params.getScalarT<UInt32>("size");
  args.activeBits = params.getScalarT<UInt32>("activeBits");
  args.sparsity =   params.getScalarT<Real32>("sparsity");
  args.radius =     params.getScalarT<Real32>("radius");
  args.resolution = params.getScalarT<Real32>("resolution");
  args.category =   params.getScalarT<bool>("category");
  args.seed =       params.getScalarT<UInt32>("seed");

  encoder_ = std::make_shared<RandomDistributedScalarEncoder>(args);
  sensedValue_ = params.getScalarT<Real64>("sensedValue");
  noise_ = params.getScalarT<Real32>("noise");
}

RDSEEncoderRegion::RDSEEncoderRegion(ArWrapper &wrapper, Region *region)
    : RegionImpl(region) {
  cereal_adapter_load(wrapper);
}
RDSEEncoderRegion::~RDSEEncoderRegion() {}
```

The primary constructor passes in the parsed parameters and a pointer to the generic Region class.  This ValueMap object contains a parsed YAML or JSON structure.  This is the parsed raw entries and does not contain default values for missing parameters or any checks for invalid parameters. So you will see the function ValidateParameters( ) which does that validation and inserts defaults.  Note: A future PR may move this function back in the engine to be called before passing the parameters to the constructor so be aware that this may change.

The next thing that happens is that we extract the parameters from the ValueMap.  It is up to you what you want to do with them but the ValueMap passed in will be deallocated when you return so you have to extract the parameters you need within the constructor.  Here we show using the legacy getScalarT< >( ) functions for each parameter. However, the ValueMap object is actually a Value object (see src/htm/ntypes/Value.hpp).  This can hold any structure that can be expressed with JSON.  The Value object has many more access functions that you can use to get the data from the ValueMap and convert from the internal strings to the data type you need.

Now that you have your parameters you can allocate internal objects for the algorithm you are wrapping. In some cases you might need to delay allocating your algorithm classes until the initialization( ) function is called indicating that configuration is finished, the buffers are allocated and its time to run.

Now, there is a second constructor that is needed to restore this class from a serialization.  Rather than passing in the parameters it passes in ArWrapper object.  You don't need to know what this does other than to just pass it to the Cereal Serialization routines.  More about that later.

### Spec
Earlier I mentioned parameter validation and default parameter values.  The engine needs to know the parameters and their properties. This information must be provided to the engine in the form of a static structure `Spec` that is returned by the function createSpec(). In the .hpp you see:
```
  static Spec *createSpec();
```

In the .cpp there are two ways to create this Spec object. You can use C++ initializers to construct the Spec object directly. (see many of the other region implementations). 

The second way to create the Spec is to create a YAML or JSON string that describes the parameters, the inputs, and the outputs that this new region will implement.  The function `Spec::parseSpec(json)` will parse it into the internal Spec structure. Parsing the JSON is slightly slower than direct initializers  but is simpler and the parse occurs only once when the region is configured.
```
/* static */ Spec *RDSEEncoderRegion::createSpec() {
  Spec *ns = new Spec();
  ns->parseSpec(R"(
  {name: "RDSEEncoderRegion",
      parameters: {
          size:        {type: UInt32, default: "0"},
          activeBits:  {type: UInt32, default: "0"},
          sparsity:    {type: Real32, default: "0.0"},
          radius:      {type: Real32, default: "0.0"},
          resolution:  {type: Real32, default: "0.0"},
          category:    {type: Bool,   default: "false"},
          seed:        {type: UInt32, default: "0"},
          noise:       {description: "amount of noise to add to the output SDR. 0.01 is 1%",
                        type: Real32, default: "0.0", access: ReadWrite },
          sensedValue: {description: "The value to encode. Overridden by input 'values'.",
                        type: Real64, default: "0.0", access: ReadWrite }},
      inputs: {
          values:      {description: "Values to encode. Overrides sensedValue.",
                        type: Real64, count: 1, isDefaultInput: true, isRegionLevel: true}}, 
      outputs: {
          bucket:      {description: "Quantized sample based on the radius. Becomes the title for this sample in Classifier.",
                        type: Real64, count: 1, isDefaultOutput: false, isRegionLevel: false },
          encoded:     {description: "Encoded bits. Not a true Sparse Data Representation (SP does that).",
                       type: SDR,    count: 0, isDefaultOutput: true, isRegionLevel: true }}
  } )");

  return ns;
}
```

In either case, the Spec has three sections.  One section is for the parameters.  Each parameter must have a name and a type and optionally a default and other things.

- name;  Use the name of the field (JSON name rules) as the label. This must be unique within the region.
- **description**; Text describing the field. Optional.
- **type**; the type of data in the parameter. For an array it is the type of an element. The types recognized are:  "Byte",   "Int16",  "UInt16", "Int32",  "UInt32", "Int64",
                 "UInt64", "Real32", "Real64", "Bool", "SDR", "String".  A type is required.  Note that the "SDR" type can be used for inputs and outputs but cannot be used for parameters.
- **count**; If it is `1` then it is a scalar. `0` means variable length array. Else a fixed size array. Default is '1', a scalar.  This is required.
- **default**; The value to use if the parameter is not provided.  This is parsed as JSON and can be a scalar or an array.  Optional.
- **access**; "Create" means this parameter can only be passed in the constructor. ReadWrite" means can be passed in constructor and set with a setter.  "ReadOnly" means can be read but not changed.  All parameters can be read. Default is "Create".

Another section is for inputs:

- name of the input as the label.
- **description**; Text describing the input. Optional.
- **type**; The expected type of the input. Same list as for parameter types. The link will convert data to this type. Required.
- **count**; size of the array to expect. '0' means a variable array size, '1' means a scalar, else expect a fixed size array.  Default is '0';
- **required**; if 'true' means there must be a link to this input. Default is 'false'. Not sure this is enforced.
- **isRegionLevel**; If 'true' this means that the dimensions on this input propagate to the default dimensions for this region.  Default is 'false';
- **isDefaultInput**; if 'true' this means that if the input name is not provided in some function then this input is assumed. Default is 'false';  Actually, we should always provide input names to functions.


The third section is for outputs:

- name of the output is the label.
- **description**; Text describing the output. Optional
- **type**; The type of the output. Same list as for parameter types. Required.
- **count**; the width of the output data. '0' means a variable sized array, '1' means a scalar, otherwise will output a fixed width array.  Default is '0';
- **isRegionLevel**; if 'true' this means obtain its dimensions from the region's default dimensions. Default is 'false';
- **isDefaultOutput**; if 'true this means that if the output name is not provided in some functions then this output is assumed. Actually, we should always provide output names to functions.  Default is 'false'.

### Dimensions and determining buffer size
The link module in the engine is responsible for moving data from the output of one region to the input of another. It automatically performs data type conversions as needed so that the input for a region is in the type expected as described in the Spec. The link module also takes care of Fan-in and Fan-out of connections.

A complete description of how dimensions are configured can be found in the `Dimensions and buffer size` section of [NetworkAPI Links](NetworkAPI_Links.md).

All of this is complicated in the engine but the Region implementation programmer only needs to implement either the askImplForOutputDimensions( ) method if you have a full dimensions, or getNodeOutputElementCount( ) if you only know what the buffer size should be.  In addition you need to be sure you have the Spec property 'isRegionLevel' set correctly on all inputs and outputs.
Everything else takes care of itself.

```
Dimensions RDSEEncoderRegion::askImplForOutputDimensions(const std::string &name) {
  if (name == "encoded") {
    // get the dimensions determined by the encoder (comes from parameters.size).
    Dimensions encoderDim(encoder_->dimensions); // get dimensions from encoder
    return encoderDim;
  }  return RegionImpl::askImplForOutputDimensions(name);
}
```

### initialize
The first phase is configuring everything. The second phase is initialization, followed by a run phase. The initialization phase occurs after the regions have been created and just before the engine starts to do the runs.  This is the point when the buffer sizes are determined and buffers created.  During initialization each region's initialize( ) function is called after creating its buffers. The region implementation can do whatever it needs to do just before execution starts, perhaps allocating the wrapped algorithm module if it could not do so in the constructor.
```
void RDSEEncoderRegion::initialize() { }
```

### execution
Each region implementation must implement the `compute( )` method.  For each iteration, this method is called once on each region in the order specified by the configuration.
```
void RDSEEncoderRegion::compute() {
  if (hasInput("values")) {
    Array &a = getInput("values")->getData();
    sensedValue_ = ((Real64 *)(a.getBuffer()))[0];
  }
  if (!std::isfinite(sensedValue_))
    sensedValue_ = 0;  // prevents an exception in case of nan or inf
  //std::cout << "RDSEEncoderRegion compute() sensedValue=" << sensedValue_ << std::endl;

  SDR &output = getOutput("encoded")->getData().getSDR();
  encoder_->encode((Real64)sensedValue_, output);

  // Add some noise.
  // noise_ = 0.01 means change 1% of the SDR for each iteration, this makes a random sequence, but seemingly stable
  if (noise_ != 0.0f)
    output.addNoise(noise_, rnd_);
    
  // output a quantized value (for use by Classifier)
  // This is a quantification of the data being encoded (the sample) 
  // and becomes the title in the Classifier.
  if (encoder_->parameters.radius != 0.0f) {
    Real64 *buf = (Real64 *)getOutput("bucket")->getData().getBuffer();
    buf[0] = sensedValue_ - std::fmod(sensedValue_, encoder_->parameters.radius);
    //std::cout << "RDSEEncoderRegion compute() bucket=" << buf[0] << std::endl;
  }
  
}
```

The region implementation accesses the buffers for each input to get its data. `getInput("values")->getData()` retrieves the Array object holding the buffer for the named input.  It can also get the Array objects for the output buffers using `getOutput("encoded")->getData()`. Calling getBuffer() will obtain the raw underlining buffer in the Array.  Note that region->getOutputData(name) or region->getInputData(name) will return const versions of the buffers.

In this case the Spec declared the output buffer named "encoded" to be an SDR type so we can access the SDR structure from the Array object by calling getSDR().

Now that it has located its buffers, the compute( ) method can then perform that task for which the region was constructed, normally by calling its wrapped algorithm module.

### Getters and Setters
If the region implementation wants to allow parameters on a region to be read after it is constructed it must implement getters.  Normally a region will allow all parameters defined in the Spec to be read.  To do this we implement a getter for each data type.  For example, here is the getter for the parameters that are of the Real32 type:
```
Real32 RDSEEncoderRegion::getParameterReal32(const std::string &name, Int64 index) {
  if (name == "resolution")    return encoder_->parameters.resolution;
  else if (name == "noise")    return noise_;
  else if (name == "radius")   return encoder_->parameters.radius;
  else if (name == "sparsity") return encoder_->parameters.sparsity;
  else return RegionImpl::getParameterReal32(name, index);
}
```
The last line is the case where there is no match with a field name and it lets the base class handle it. There will need to be one of these for each data type that you use in the Spec.

The region implementation may also allow some parameters to be changed after construction.  These should be marked as ReadWrite access in the Spec.  For each data type, the region implementation must implement setters that handle assignment for all of the parameters of that type.  For example, here are the setters for the Real32 parameters:
```
void RDSEEncoderRegion::setParameterReal32(const std::string &name, Int64 index, Real32 value) {
  if (name == "noise") noise_ = value;
  else RegionImpl::setParameterReal32(name, index, value);
}
```

Again, if there is no match with the parameter name, it should be passed to the base class for handling.

### Serialization
We are using Cereal Serialization.  The objective is to save all state (i.e. parameters) that would be needed to continue executing in the next call to compute( ) following a restore.  The constructor section talked a little about a second constructor for deserialization.  In addition to that constructor you will need some lines to tell it what needs to be saved (and restored).  Here is the serialization from our example region:

```
  CerealAdapter;  // see Serializable.hpp
  // FOR Cereal Serialization
  template<class Archive>
  void save_ar(Archive& ar) const {
    ar(CEREAL_NVP(sensedValue_));
    ar(CEREAL_NVP(noise_));
    ar(CEREAL_NVP(rnd_));
    ar(cereal::make_nvp("encoder", encoder_));
  }
  // FOR Cereal Deserialization
  // NOTE: the Region Implementation must have been allocated
  //       using the RegionImplFactory so that it is connected
  //       to the Network and Region objects. This will populate
  //       the region_ field in the Base class.
  template<class Archive>
  void load_ar(Archive& ar) {
    ar(CEREAL_NVP(sensedValue_));
    ar(CEREAL_NVP(noise_));
    ar(CEREAL_NVP(rnd_));
    ar(cereal::make_nvp("encoder", encoder_));
    setDimensions(encoder_->dimensions); 
  }
  ```
  
  `CerealAdapter` is a macro that tries to hide some of the common details of getting the ar variable. Just use it as is.  Then there is a save_ar(ar) function and a load_ar(ar) function.  The `sensedValue_`, `noise_`, and `rnd_` variables are scalers in our example so just replace those with your own scalars that need to be saved. The Cereal package knows how to save STL objects as well, but not pointers. You may need some special handling before saving or after restoring such as that being performed by the setDimensions( ) function in the above example.
  
  Note that if your implementation wraps some other algorithm that contains state that needs to be restored then you need to serialize that algorithm as well. For example `ar(cereal::make_nvp("encoder", encoder_));` will perform the Cereal functions on the encoder algorithm. So it must also contain the save_ar( ) and load_ar( ) functions.

### Other things
It is common practice to provide an equal operator for comparing two instances of your class.  The streaming operator << might be useful as well.

Do not forget to include a full Unit Test for your new region.  It cannot be accepted into the htm.core without a full review and a working unit test.
