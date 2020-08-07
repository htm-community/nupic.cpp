/** 
 * Parallelizable.hpp
 *
 * include this header to files where you want to run blocks of code in parallel. 
 *  We use [C++17 standard Parallel TS](https://en.cppreference.com/w/cpp/experimental/parallelism). 
 *
 * Requirements: 
 *  - c++17
 *  - The Building Blocks (tbb) linked to the library
 *  - [supported compiler](https://en.cppreference.com/w/cpp/compiler_support#cpp17): currently GCC-9+, MSVC 2019
 * //TODO: switch to c++17 by default, or implement `transform()` temporarily as a custom method? 
 *
 * Functionality:
 *  - include all needed headers for given platform, compiler, ... 
 *  - handle define `NUM_PARALLEL=n`
 *    - to run in single thread, set NUM_PARALLEL=1
 *
 */  

//includes for TS Parallel
#include <execution> //std::execution::par, seq, par_unseq
// #include <tbb/parallel_for.h> 
#include <mutex>

namespace htm {
namespace parallel {
  const constexpr auto mode = std::execution::par_unseq; //TODO ifdef NUM_PARALLEL=1 -> seq
}
}
