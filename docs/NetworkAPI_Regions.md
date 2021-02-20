
# NetworkAPI Predefined Region Types
There are a number of predefined C++ region implementations that are included in the htm.core library. These do not need to be registered.  There are also Python implemented regions and user written C++ regions that can act as plugins. These will need to be registered so they can be loaded at run-time.

The built-in C++ region implementations that are included in the htm.core library are:
- ScalarEncoderRegion  - encodes numeric and category data
- RDSEEncoderRegion   - encodes numeric and category data using a hash
- DateEncoderRegion   - encodes date and/or time data
- SPRegion      - HTM Spatial Pooler implementation
- TMRegion      - HTM Temporal Memory implementation
- FileOutputRegion  - Writes data to a file
- FileInputRegion   - Reads data from a file
- ClassifierRegion  - An SDR classifier



## ScalarEncoderRegion
A ScalarEncoderRegion encapsulates the ScalarEncoder algorithm, an SDR encoder for numeric or category values. As a network runs, the client will specify new encoder inputs by setting the "sensedValue" parameter or connect a data stream to its 'values' input. An assignment to the 'sensedValue' parameter will override the input stream. On each compute cycle, the ScalarSensor will encode its input into the output. The algorithom determines the number of buckets and then encodes input such that the input results in the required number of bits in the resulting pattern such that there are no overlapping bits for values in a bucket.   See HTM School videos for more insite into how this works.

These four (4) members define the total number of bits in the output pattern:
- size (or n),
- radius,
- category,
- resolution.

These are mutually exclusive and only one of them should be provided when
constructing the encoder.

These two (2) members define the number of bits per bucket in the output:
- sparsity,
- activeBits (or w).

These are also mutually exclusive and only one of them should be provided when constructing the encoder.

Note that the output is an array of boolean values in a pattern and is output in an SDR.  However, this is not a true SDR in that it is not sparse. This pattern should be passed to an SPRegion to set the sparsity.


<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> sensedValue  </td><td> An input value. </td><td> ReadWrite </td><td> Real64 </td><td> -1 </td></tr>
<tr><td> size (or n)</td><td> Width of encoding (total number of boolean values in the output). </td><td> Create </td><td> UInt32 </td><td width=10%>(required entry)
<tr><td> activeBits (or w)  </td><td> Member "activeBits" is the number of true bits in the encoded output SDR.
    Each output encoding will have a contiguous block of this many 1's. </td><td> Create </td><td> UInt32 </td><td width=10%>(required entry)
<tr><td> sparsity  </td><td> Member "sparsity" is an alternative way to specify the member "activeBits".
     Sparsity requires that the size to also be specified.
     Specify only one of: activeBits or sparsity. </td><td> Create </td><td> Real32 </td><td width=10%>(required entry)
<tr><td> resolution  </td><td>The resolution for the encoder. How wide is the quantized bucket.</td><td> Create </td><td> Real64 </td><td width=10%>(required entry)
<tr><td> radius  </td><td>How many buckets are there in the range of possible input values.</td><td> Create </td><td> Real64 </td><td width=10%>(required entry)
<tr><td> category  </td><td> Member "category" means that the inputs are enumerated categories.
 If true then this encoder will only encode unsigned integers, and all
 inputs will have unique / non-overlapping representations. </td><td> Create </td><td> Boolean </td><td width=10%>false
<tr><td> minValue  </td><td>The smallest input value expected.</td><td> Create </td><td> Real64 </td><td width=10%>-1.0
<tr><td> maxValue  </td><td>The largest input value expected</td><td> Create </td><td> Real64 </td><td width=10%>+1.0
<tr><td> periodic  </td><td>Does the pattern repeat.</td><td> Create </td><td> Boolean </td><td width=10%>false
<tr><td> clipInput  </td><td>Should out-of-range values be clipped to minValue or maxValue? Else it gives an error.</td><td> Create </td><td> Boolean </td><td width=10%>false
</table>

<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> values   </td><td>The value to be encoded for the current sample.  </td><td> Real64    </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> encoded   </td><td>The encoded bits forming the pattern for this sample. </td><td> SDR    </td></tr>
<tr><td> bucket   </td><td>The quantized input.  The value of the current bucket. This is used by the Classifier while learning. </td><td> Real64    </td></tr>
</table>



## RDSEEncoderRegion
A RDSEEncoderRegion region encoder encapsulates the RandomDistributedScalarEncoder(RDSE) algorithm, an SDR encoder for numeric or category values. This is similar to the ScalarSensor region encoder except that the minimum and maximum values do not need to be known and it generates a hash that is encoded thus giving a better spread of values.

As a network runs, the client will specify new encoder inputs by setting the "sensedValue" parameter or connect a data stream to its 'values' input. An assignment to the 'sensedValue' parameter will override the input stream if both are given. On each compute cycle, the ScalarSensor will encode its input into the output. The algorithom hashes the input and the hashed value determines the number of buckets. It then encodes this hashed value such that the input results in the required number of bits in the resulting pattern such that there are no overlapping bits for values in a bucket. 

These four (3) members define the total number of bits in the output pattern:
- radius,
- category,
- resolution.

These are mutually exclusive and only one of them should be provided when
constructing the encoder.

These two (2) members define the number of bits per bucket in the output:
- sparsity,
- activeBits.

These are also mutually exclusive and only one of them should be provided when
constructing the encoder.

Note that the output is an array of boolean values in a pattern and is output in an SDR.  However, this is not a true SDR in that it is not sparse. This pattern should be passed to an SPRegion to set the sparsity.

<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> sensedValue  </td><td> An input value. </td><td> ReadWrite </td><td> Real64 </td><td> -1 </td></tr>
<tr><td> size</td><td> Width of encoding (total number of boolean values in the output). </td><td> Create </td><td> UInt32 </td><td width=10%>(required entry)
<tr><td> activeBits  </td><td> Member "activeBits" is the number of true bits in the encoded output SDR.
    Each output encoding will have a contiguous block of this many 1's. </td><td> Create </td><td> UInt32 </td><td width=10%>(required entry)
<tr><td> sparsity  </td><td> Member "sparsity" is an alternative way to specify the member "activeBits".
     Sparsity requires that the size to also be specified.
     Specify only one of: activeBits or sparsity. </td><td> Create </td><td> Real32 </td><td width=10%>(required entry)
<tr><td> resolution  </td><td>The resolution for the encoder. How wide is the quantized bucket.</td><td> Create </td><td> Real64 </td><td width=10%>(required entry)
<tr><td> radius  </td><td>Member "radius" Two inputs separated by more than the radius have
   non-overlapping representations. Two inputs separated by less than the
   radius will in general overlap in at least some of their bits. You can
   think of this as the radius of the input.</td><td> Create </td><td> Real64 </td><td width=10%>(required entry)
<tr><td> category  </td><td> Member "category" means that the inputs are enumerated categories.
 If true then this encoder will only encode unsigned integers, and all
 inputs will have unique / non-overlapping representations. </td><td> Create </td><td> Boolean </td><td width=10%>false
<tr><td> seed  </td><td>Member "seed" forces different encoders to produce different outputs, even
   if the inputs and all other parameters are the same.  Two encoders with the
   same seed, parameters, and input will produce identical outputs.
   The seed 0 is special.  Seed 0 is replaced with a random number. Use a non-zero value if you want the results to be reproducible.</td><td> Create </td><td> UInt32 </td><td width=10%>0
<tr><td> noise  </td><td>amount of noise to add to the output SDR. 0.01 is 1%. </td><td> Create </td><td> Real64 </td><td width=10%>0
</table>

<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> values   </td><td>The value to be encoded for the current sample.  </td><td> Real64    </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> encoded   </td><td>The encoded bits forming the pattern for this sample. </td><td> SDR    </td></tr>
<tr><td> bucket   </td><td>The quantized input.  The value of the current bucket. This is used by the Classifier while learning. </td><td> Real64    </td></tr>
</table>

## DateEncoderRegion
The DateEncoderRegion region encapsulates the DateEncoder algorithm.
It encodes up to 6 attributes of a timestamp value into an array of 0's and 1's.
 
The input is a timestamp which is unix date/time; an integral value representing the number of
seconds elapsed since 00:00 hours, Jan 1, 1970 UTC (the unix EPOCH).  Some platforms (unix and linux)
allow negitive numbers as the timestamp which allows time before EPOCH to be expressed.
However some platforms (windows) allow only positive numbers.  If the type time_t on your computer
is is 32bits then the timestamp will not allow dates after Jan 18, 2038. By default, on windows
it is 64bit but on some older 32bit linux machines time_t is 32bit. google "Y2K38".

The output is an array containing 0's except for a contiguous block of 1's for each
attribute member. This is held in an SDR container although technically this is not
a sparse representation. It is normally passed to a SpatialPooler which will turn
 this into a true sparse representation.
 
The *_width parameters determine if an attribute is used in the encoding. If non-zero, it indicates the number of bits to dedicate to this attribute.  The more bits specified relative to the other attributes, the more important that attribute is in the resulting semantic pattern.
 
<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> season_width  </td><td> How many bits apply to the season attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> season_radius  </td><td> How many days are considered a season. </td><td> Create </td><td> Real32 </td><td> 91.5 </td></tr>
<tr><td> dayOfWeek_width  </td><td> How many bits apply to the day-of-week attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> dayOfWeek_radius  </td><td> How many days are considered a day of week. </td><td> Create </td><td> Real32 </td><td> 1.0 </td></tr>
<tr><td> weekend_width  </td><td> How many bits apply to the weekend attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> timeOfDay_width  </td><td> How many bits apply to the timeOfDay attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> timeOfDay_radius  </td><td> How many hours in a time-of-day bucket. </td><td> Create </td><td> Real32 </td><td> 4.0 </td></tr>
<tr><td> custom_width  </td><td> How many bits apply to the custom day attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> custom_days  </td><td> A list of day-of-week to be included in the set, i.e. 'mon,tue,fri' </td><td> Create </td><td> String </td><td>  </td></tr>
<tr><td> holiday_width  </td><td> How many bits apply to the holiday attribute. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> holiday_days  </td><td>A JSON encoded list of holiday dates in format of '[month,day]' or '[year,month,day]', ie for two dates, [[12,25],[2020,05,04]] </td><td> Create </td><td> String </td><td> [[12,25]] </td></tr>
<tr><td> verbose  </td><td> if true, display debug info for each member encoded. </td><td> ReadWrite </td><td> Boolean </td><td> false </td></tr>
<tr><td> size  </td><td> Total width of encoded output</td><td> ReadOnly </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> noise  </td><td> The amount of noise to add to the output SDR. 0.01 is 1%" </td><td> ReadWrite </td><td> Real32 </td><td> 0.0 </td></tr>
<tr><td> sensedTime  </td><td>The value to encode. Unix EPOCH time. Overriden by input 'values'. A value of 0 means current time. </td><td> ReadWrite </td><td> Int64 </td><td> 0.0 </td></tr></table>
 
 
<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> values   </td><td>The value to be encoded for the current sample.  </td><td> Int64 (time_t)    </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bucket   </td><td>Quantized samples based on the radius. One sample for each attribute used. Becomes the title for this sample in the Classifier. </td><td> Real64    </td></tr>
<tr><td> encoded   </td><td>The encoded bits forming the pattern for this sample. </td><td> SDR    </td></tr>
</table>


## SPRegion
The SPRegion encapsulates the HTM Spatial Pooler algorithm 
      described in 'BAMI 
      <https://numenta.com/biological-and-machine-intelligence/>'.   The Spatial Pooler is responsible for creating a sparse distributed representation of the input. Given an input it computes a set of sparse
active columns and simultaneously updates its permanence, duty cycles, etc.

<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> columnCount  </td><td> Total number of columns (coincidences). This is the Output Dimension for the SDR. </td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> inputWidth  </td><td>Maximum size of the 'bottomUpIn' input to the SP. 
                    This is the input Dimension. The input buffer width
                    is taken from the width of all concatinated output 
                    buffers that are connected to the input. </td><td> ReadOnly </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> potentialRadius  </td><td> This parameter determines the extent of the input that 
                    each column can potentially be connected to. This can 
                    be thought of as the input bits that are visible to 
                    each column, or a 'receptiveField' of the field of 
                    vision. A large enough value will result in 'global 
                    coverage', meaning that each column can potentially 
                    be connected to every input bit. This parameter defines 
                    a square(or hyper square) area: a column will have a 
                    max square potential pool with sides of length 
                    '2 * potentialRadius + 1'. Default '0'. 
                    If 0, during, Initialization it is set to the value of 
                    inputWidth parameter. </td><td> ReadWrite </td><td> UInt32 </td><td> 16 </td></tr>
<tr><td> potentialPct  </td><td> The percent of the inputs, within a column's potential radius, 
          that a column can be connected to.If set to 1, the column will be connected 
          to every input within its potential radius.This parameter is used to 
          give each column a unique potential pool when a large potentialRadius 
          causes overlap between the columns.At initialization time we choose 
          ((2 * potentialRadius + 1) ^ (# inputDimensions) * potentialPct) input bits 
          to comprise the column's potential pool.</td><td> ReadWrite </td><td> Real32 </td><td> 0.5 </td></tr>
<tr><td> globalInhibition  </td><td> If true, then during inhibition phase the winning columns are 
          selected as the most active columns from the region as a whole.Otherwise, the 
          winning columns are selected with respect to their local neighborhoods. 
          Using global inhibition boosts performance x60. </td><td> ReadWrite </td><td> Boolean </td><td> true </td></tr>
<tr><td> localAreaDensity  </td><td>The desired density of active columns within a local inhibition 
          area (the size of which is set by the internally calculated 
          inhibitionRadius,  which is in turn determined from the average 
          size of the connected potential pools of all columns). The 
          inhibition logic will insure that at most N columns remain ON 
          within a local inhibition area, where N = localAreaDensity * 
          (total number of columns in inhibition area).  </td><td> ReadWrite </td><td> Real32 </td><td> 0 </td></tr>
<tr><td> stimulusThreshold  </td><td> This is a number specifying the minimum 
                    number of synapses that must be 
                    on in order for a columns to turn ON.The 
                    purpose of this is to prevent 
                    noise input from activating 
                    columns.Specified as a percent of a fully 
                     grown synapse. </td><td> ReadWrite </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> synPermInactiveDec  </td><td> The amount by which an inactive synapse is decremented in each 
          round. Specified as a percent of a fully grown synapse. </td><td> ReadWrite </td><td> Real32 </td><td> 0.008 </td></tr>
<tr><td> synPermActiveInc  </td><td>The amount by which an active synapse is incremented in each round.
          Specified as a percent of a fully grown synapse. </td><td> ReadWrite </td><td> Real32 </td><td> 0.05 </td></tr>
<tr><td> synPermConnected  </td><td>The default connected threshold.Any synapse whose 
                    permanence value is above the connected threshold is a 'connected synapse', 
                    meaning it can contribute to the cell's firing.  </td><td> ReadWrite </td><td> Real32 </td><td> 0.1 </td></tr>
<tr><td> minPctOverlapDutyCycles  </td><td> A number between 0 and 1.0, used to set a 
          floor on how often a column 
          should have at least stimulusThreshold active inputs.Periodically, each 
          column looks at the overlap duty cycle of all other columns within its 
          inhibition radius and sets its own internal minimal acceptable duty 
          cycle to : minPctDutyCycleBeforeInh * max(other columns' duty cycles).  
          On each iteration, any column whose overlap duty cycle falls below this 
          computed value will  get all of its permanence values boosted up by 
          synPermActiveInc. Raising all permanences in response to a sub-par 
          duty cycle before  inhibition allows a cell to search for new inputs 
          when either its previously learned inputs are no longer ever active, or 
          when the vast majority of them have been 'hijacked' by other 
          columns. </td><td> ReadWrite </td><td> Real32 </td><td> 0.001 </td></tr>
<tr><td> dutyCyclePeriod  </td><td> The period used to calculate duty cycles.Higher 
          values make it take longer to respond to changes in boost or synPerConnectedCell. 
          Shorter values make it more unstable and likely to oscillate. </td><td> ReadWrite </td><td> Real32 </td><td> 0.0 </td></tr>
<tr><td> boostStrength  </td><td> A number greater or equal than 0.0, used to control the strength of 
          boosting. No boosting is applied if it is set to 0. Boosting strength 
          increases as a function of boostStrength.Boosting encourages columns to 
          have similar activeDutyCycles as their neighbors, which will lead to more 
          efficient use of columns. However, too much boosting may also lead to 
          instability of SP outputs. </td><td> ReadWrite </td><td> Real32 </td><td> 0.0 </td></tr>
<tr><td> seed  </td><td>Seed for our own pseudo - random number generator. </td><td> Create </td><td> Int32 </td><td> 1 </td></tr>
<tr><td> spVerbosity  </td><td>Verbosity level : 0, 1, 2, or 3 for SP.  </td><td> ReadWrite </td><td> Int32 </td><td> 0 </td></tr>
<tr><td> wrapAround  </td><td>Determines if inputs at the beginning and 
          end of an input dimension should be considered neighbors when mapping 
          columns to inputs.</td><td> ReadWrite </td><td> Boolean </td><td> true </td></tr>
<tr><td> spInputNonZeros  </td><td>The indices of the non-zero inputs to the spatial pooler. </td><td> ReadOnly </td><td> SDR </td><td>  </td></tr>
<tr><td> spOutputNonZeros  </td><td>The indices of the non-zero outputs from the spatial pooler </td><td> ReadOnly </td><td> SDR </td><td>  </td></tr>
<tr><td> learningMode  </td><td>True if the node is in learning mode.</td><td> ReadWrite </td><td> Boolean </td><td> true </td></tr>
<tr><td> activeOutputCount  </td><td>Number of active elements in bottomUpOut output.</td><td> ReadOnly </td><td> UInt32 </td><td> 0 </td></tr>
</table>
 
<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bottomUpIn   </td><td>The input pattern. </td><td> SDR </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bottomUpOut   </td><td>The output pattern generated from bottom-up inputs. </td><td> SDR    </td></tr>
</table>


## TMRegion
The TMRegion region implements the temporal memory algorithm as 
      described in 'BAMI 
      <https://numenta.com/biological-and-machine-intelligence/>'.  
      The implementation here attempts to closely match the pseudocode in 
      the documentation. 
      
<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> numberOfCols  </td><td> Number of mini-columns in the region. This values 
                    needs to be the same as the number of columns in the 
                    input from SP.  Normally this value is derived from 
                    the input width but if provided, this parameter must be 
                    the same total size as the input.</td><td> Create </td><td> UInt32 </td><td> (required input) </td></tr>
<tr><td> cellsPerColumn  </td><td>The number of cells per mini-column. </td><td> Create </td><td> UInt32 </td><td> 32 </td></tr>
<tr><td> activationThreshold  </td><td> Number of synapses that must be active to 
                    activate a segment. </td><td> Create </td><td> UInt32 </td><td> 13 </td></tr>
<tr><td> initialPermanence  </td><td> Initial permanence for newly created synapses. </td><td> Create </td><td> Real32 </td><td> 0.21 </td></tr>
<tr><td> connectedPermanence  </td><td>  </td><td> Create </td><td> Real32 </td><td> 0.5 </td></tr>
<tr><td> minThreshold  </td><td> Minimum number of active synapses for a segment to 
          be considered during search for the best-matching segments. </td><td> Create </td><td> UInt32 </td><td> 10 </td></tr>
<tr><td> maxNewSynapseCount  </td><td> The max number of synapses added 
                    to a segment during learning. </td><td> Create </td><td> UInt32 </td><td> 20 </td></tr>
<tr><td> permanenceIncrement  </td><td> Active synapses get their permanence counts 
                    incremented by this value. </td><td> Create </td><td> Real32 </td><td> 0.1 </td></tr>
<tr><td> permanenceDecrement  </td><td>  All other synapses get their permanence counts 
                    decremented by this value. </td><td> Create </td><td> Real32 </td><td> 0.1 </td></tr>
<tr><td> predictedSegmentDecrement  </td><td>Predicted segment decrement  A good value 
                    is just a bit larger than (the column-level sparsity * 
                    permanenceIncrement). So, if column-level sparsity is 2% 
                    and permanenceIncrement is 0.01, this parameter should be 
                    something like 4% * 0.01 = 0.0004). </td><td> Create </td><td> Real32 </td><td> 0.0 </td></tr>
<tr><td> maxSegmentsPerCell  </td><td>  The maximum number of segments allowed on a 
                  cell. This is used to turn on 'fixed size CLA' mode. When in 
                  effect, 'globalDecay' is not applicable and must be set to 0 and 
                  'maxAge' must be set to 0. When this is used (> 0), 
                  'maxSynapsesPerSegment' must also be > 0.  </td><td> ReadOnly </td><td> UInt32 </td><td> 255 </td></tr>
<tr><td> maxSynapsesPerSegment  </td><td> The maximum number of synapses allowed in 
                    a segment. </td><td> ReadWrite </td><td> UInt32 </td><td> 255 </td></tr>
<tr><td> seed  </td><td> Random number generator seed. The seed affects the random 
                    aspects of initialization like the initial permanence values. A 
                    fixed value ensures a reproducible result.</td><td> Create </td><td> UInt32 </td><td> 42 </td></tr>
<tr><td> inputWidth  </td><td> width of bottomUpIn Input. Will be 0 if no links. </td><td> ReadOnly </td><td> UInt32 </td><td>  </td></tr>
<tr><td> learningMode  </td><td>True if the node is learning.  </td><td> Create </td><td> Boolean </td><td> true </td></tr>
<tr><td> activeOutputCount  </td><td> Number of active elements. </td><td> ReadOnly </td><td> UInt32 </td><td>  </td></tr>
<tr><td> anomaly  </td><td> The anomaly Score computed for the current iteration. 
                   This is the same as the output anomaly. </td><td> ReadOnly </td><td> Real32 </td><td>  </td></tr>
<tr><td> orColumnOutputs  </td><td>True if the bottomUpOut is to be aggregated by column 
                    by logical ORing all cells in a column.  Note that if this 
                    is on, the buffer width is numberOfCols rather than 
                    numberOfCols * cellsPerColumn so must be set prior 
                    to initialization.  </td><td> Create </td><td> Boolean </td><td> false </td></tr>
</table>

 
<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bottomUpIn   </td><td>The input signal, conceptually organized as an image pyramid 
                data structure, but internally organized as a flattened vector. 
                The width should match the output of SP.  Set numberOfCols to 
                This value if not configured.  Otherwise the parameter overrides. </td><td> SDR </td></tr>
<tr><td> externalPredictiveInputsActive   </td><td>External extra active bits from an external source. 
                These can come from anywhere and be any size. If provided, 
                the 'externalPredictiveInputs' flag is set to dense buffer size and both 
                extraActive and extraWinners must be provided and have the
                same dense buffer size.  Dimensions are set by source. </td><td> SDR </td></tr>
<tr><td> externalPredictiveInputsWinners   </td><td>The winning active bits from an external source. 
                These can come from anywhere and be any size. If provided, 
                the 'externalPredictiveInputs' flag is set to dense buffer size and both 
                extraActive and extraWinners must be provided and have the
                same dense buffer size.  Dimensions are set by source. </td><td> SDR </td></tr>
<tr><td> resetIn   </td><td>A boolean flag that indicates whether 
                or not the input vector received in this compute cycle 
                represents the first training presentation in new temporal 
                sequence. </td><td> SDR </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bottomUpOut   </td><td>The output signal generated from the bottom-up inputs 
                 from lower levels. The width is 'numberOfCols' 
                 * 'cellsPerColumn' by default; if orColumnOutputs is 
		 set, then this returns only numberOfCols. 
		 The activations come from TM::getActiveCells().  </td><td> SDR    </td></tr> 
		 
<tr><td> activeCells   </td><td>The cells that are active from TM computations. 
                 The width is 'numberOfCols' * 'cellsPerColumn'.</td><td> SDR    </td></tr> 
<tr><td> predictedActiveCells   </td><td>The cells that are active and predicted, the winners. 
                 The width is 'numberOfCols' * 'cellsPerColumn'. </td><td> SDR   </td></tr>  
<tr><td> anomaly   </td><td>The anomaly score for current iteration. </td><td> Real32  </td></tr>   
<tr><td> predictiveCells   </td><td>The cells that are predicted. 
                 The width is 'numberOfCols' * 'cellsPerColumn'. </td><td> SDR    </td></tr>
</table>



## FileOutputRegion
FileOutputRegion is a region that takes its input stream and
writes it sequentially to a file. Note: this originally was VectorFileEffector.
The current input is written (but not flushed) to the file
each time the effector is executed.
The file format for the file is a space-separated list of numbers, with
one vector per line:
   ```
          e11 e12 e13 ... e1N
          e21 e22 e23 ... e2N
             :
          eM1 eM2 eM3 ... eMN
   ```
   
<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> outputFile  </td><td> Writes output vectors to this file on each 
                            run iteration. Will append to any 
                            existing data in the file. This parameter 
                            must be set at runtime before 
                            the first compute is called. Throws an 
                            exception if it is not set or 
                            the file cannot be written to. </td><td> ReadWrite </td><td> String </td><td> (required input) </td></tr>
</table>

 
<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> dataIn   </td><td>The data to be written to the file. </td><td> SDR </td></tr>
</table>

<table>
<tr><th> command </th><th>  Description  </th></tr>
<tr><td> flushFile   </td><td> Flush file data to disk without closing.</td></tr>
<tr><td> closeFile   </td><td>close the file.</td></tr>
</table>


## FileInputRegion
FileInputRegion is a basic sensor region for reading files containing vectors.
Note: this originally was VectorFileSensor.
      
FileInputRegion reads in a text file containing lists of numbers
and outputs these vectors in sequence. The output is updated
each time the sensor's compute() method is called. If
repeatCount is > 1, then each vector is repeated that many times
before moving to the next one. The sensor loops when the end of
the vector list is reached. The default file format
is as follows (assuming the sensor is configured with N outputs):
      ```
        e11 e12 e13 ... e1N
        e21 e22 e23 ... e2N
          : 
        eM1 eM2 eM3 ... eMN
      ```
In this format the sensor ignores all whitespace in the file, including newlines.
If the file contains an incorrect number of floats, the sensor has no 
way of checking and will silently ignore the extra numbers at the end of 
the file.

The sensor can also read in comma-separated (CSV) files following the format:
      ```
        e11, e12, e13, ... ,e1N
        e21, e22, e23, ... ,e2N
          : 
        eM1, eM2, eM3, ... ,eMN
      ```
When reading CSV files the sensor expects that each line contains a new vector.
Any line containing too few elements or any text will be ignored. If 
there are more than N numbers on a line, the sensor retains only the first N.
<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access </td><td> Type </td><td>Default </td></tr>
<tr><td> vectorCount  </td><td> The number of vectors currently loaded in memory.</td><td> ReadOnly </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> position  </td><td> Set or get the current position within the 
                             list of vectors in memory. Index of vector last output. 
                             Before anything is output, position is -1. Setting a position 
                             will position just prior to the requested vector so that 
                             the requested value will be the next output from the next 
                             call to compute(). If the requested position is outside the 
                             range of the vector set, it will wrap to be inside the range.</td><td> ReadWrite </td><td> UInt32 </td><td> -1 </td></tr>
<tr><td> repeatCount  </td><td> Set or get the current repeatCount. Each vector is repeated\n"
					          "repeatCount times before moving to the next one.</td><td> ReadWrite </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> recentFile  </td><td>Writes output vectors to this file on each 
                                   iteration. Will append to any
                                   existing data in the file. This parameter 
                                   must be set at runtime before
                                   the first compute is called. Throws an 
                                   exception if it is not set or
                                   the file cannot be written to. </td><td> ReadOnly </td><td> String </td><td>  </td></tr>
<tr><td> scalingMode  </td><td>During an iteration, each vector is adjusted as follows. If X is the 
				data vector, S the scaling vector and O the offset vector, then the node's output</br>
				                Y[i] = S[i]*(X[i] + O[i]).<br>
				Scaling is applied according to scalingMode as follows: <br>

				If 'none', the vectors are unchanged, i.e. S[i]=1 and O[i]=0.<br>
				If 'standardForm', S[i] is 1/standard deviation(i) and O[i] = - mean(i)<br>
				If 'custom', each component is adjusted according to the vectors specified by the setScale and setOffset commands.</td><td> ReadWrite </td><td> String </td><td> 0 </td></tr>
<tr><td> scaleVector  </td><td> Set or return the current scale vector S.</td><td> ReadWrite </td><td> Real32 array </td><td> 0 </td></tr>
<tr><td> offsetVector  </td><td> Set or return the current offset vector.</td><td> ReadWrite </td><td> Real32 </td><td> 0 </td></tr>
<tr><td> activeOutputCount  </td><td>The number of active outputs of the node.</td><td> Create </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> maxOutputVectorCount  </td><td> The number of output vectors that can be generated by this sensor under the current configuration.</td><td> ReadOnly </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> hasCategoryOut  </td><td>If 1, category info is present in data file.</td><td> ReadWrite </td><td> UInt32 </td><td> 0 </td></tr>
<tr><td> hasResetOut  </td><td>If 1, a new sequence reset signal is present in data file.</td><td> ReadWrite </td><td> UInt32 </td><td> 0 </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> dataOut   </td><td>The data read from the file. </td><td> Real32 </td></tr> 
</table>

<table>
<tr><th> command </th><th>  Description  </td></tr>
<tr><td> loadFile   </td><td>loadFile &lt;filename&gt; [file_format]<br>
        Reads vectors from the specified file, replacing any vectors
        currently in the list. Position is set to zero.
        Available file formats are:<br>
         0 - Reads in unlabeled file with first number = element count<br>
         1 - Reads in a labeled file with first number = element count (deprecated)<br>
         2 - Reads in unlabeled file without element count (default)<br>
         3 - Reads in a csv file<br> </td></tr> 
<tr><td> appendFile   </td><td>appendFile &lt;filename&gt; [file_format]<br>
Reads vectors from the specified file, appending to current vector list.
        Position remains unchanged. Available file formats are the same as for the loadFile command.
<tr><td> saveFile   </td><td>saveFile &lt;filename&gt; [format [begin [end]]]<br>
Save the currently loaded vectors to a file. Typically used for debugging
but may be used to convert between formats. </td></tr> 
<tr><td> Dump   </td><td>return debugging information. </td></tr> 
</table>


## ClassifierRegion
This is a wrapper around the SDRClassifier algorithm. Used to map SP and TM output back to original entries. The SDR Classifier takes the form of a single layer classification network (NN). It accepts SDRs as input and outputs a predicted distribution of categories.

Note that this will require many samples of every quantized value or category during the learning phase in order to get reasonable results.
<table>
<tr><th> Parameter </th><th>  Description  </th><th>  Access   </td><td> Type </td><td>Default </td></tr>
<tr><td> learn  </td><td> If true, the classifier is in learning mode. </td><td> ReadWrite </td><td> Boolean </td><td> true</td></tr>
</table>

<table>
<tr><th> Input </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> bucket   </td><td>The quantized value of the current sample, one from each encoder if more than one, for the learn step     </td><td> Real64    </td></tr>
<tr><td> pattern   </td><td> An SDR output bit pattern for a sample.  Usually the output of the SP or TM. </td><td> SDR  </td></tr>
</table>

<table>
<tr><th> Output </th><th>  Description  </th><th>  Data Type   </td></tr>
<tr><td> pdf   </td><td>probability distribution function (pdf) for each category or bucket. Sorted by title.  Warning, buffer length will grow as new bucket values are encountered while learning. </td><td> Real64    </td></tr>
<tr><td> titles </td><td>Quantized values of used samples which are the Titles corresponding to the pdf indexes. Sorted by title. Warning, buffer length will grow with pdf.</td><td> Real64 </td></tr>
<tr><td> predicted </td><td>An index (into pdf and titles) with the highest probability of being the match with the current pattern.</td><td> UInt32 </td></tr>
</table>
