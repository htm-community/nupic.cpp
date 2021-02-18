/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013-2015, Numenta, Inc.
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

#include <algorithm> // std::generate
#include <iostream>
#include <vector>

#include "HelloSPTP.hpp"

#include "htm/algorithms/TemporalMemory.hpp"
#include "htm/algorithms/SpatialPooler.hpp"
#include "htm/encoders/RandomDistributedScalarEncoder.hpp"
#include "htm/algorithms/AnomalyLikelihood.hpp"

#include "htm/types/Sdr.hpp"
#include "htm/utils/Random.hpp"
#include "htm/utils/MovingAverage.hpp"
#include "htm/utils/SdrMetrics.hpp"

namespace examples {

using namespace std;
using namespace htm;


// work-load
Real64 BenchmarkHelloSPTP::run(UInt EPOCHS, bool useSPlocal, bool useSPglobal, bool useTM, const UInt COLS, const UInt DIM_INPUT, const UInt CELLS)
{
#ifndef NDEBUG
EPOCHS = 2; // make test faster in Debug
#endif

#if defined __aarch64__ || defined __arm__
#undef _ARCH_DETERMINISTIC
#else
#define _ARCH_DETERMINISTIC
#endif

  if(useTM ) {
	  NTA_CHECK(useSPlocal or useSPglobal) << "using TM requires a SP too";
  }

  std::cout << "starting test. DIM_INPUT=" << DIM_INPUT
  		<< ", DIM=" << COLS << ", CELLS=" << CELLS << std::endl;
  std::cout << "EPOCHS = " << EPOCHS << std::endl;


  // initialize SP, TM, AnomalyLikelihood
  tInit.start();
  RDSE_Parameters encParams;
  encParams.sparsity = 0.2f; //20% of the encoding are active bits (1's)
  encParams.size = DIM_INPUT; //the encoder is not optimal, it's to stress-test the SP,TM
//  encParams.resolution = 0.002f;
  encParams.radius = 0.03f;
  encParams.seed = 2019u;
  RandomDistributedScalarEncoder enc( encParams );
  SpatialPooler spGlobal(enc.dimensions, vector<UInt>{COLS}); // Spatial pooler with globalInh
  SpatialPooler  spLocal(enc.dimensions, vector<UInt>{COLS}); // Spatial pooler with local inh
  spGlobal.setGlobalInhibition(true);
  spLocal.setGlobalInhibition(false);
  //spLocal.setWrapAround(false);
  Random rnd(42); //uses fixed seed for deterministic output checks

  TemporalMemory tm(vector<UInt>{COLS}, CELLS);

  AnomalyLikelihood anLikelihood;
  tInit.stop();

  // data for processing input
  SDR input(enc.dimensions);
  SDR outSPglobal(spGlobal.getColumnDimensions()); // active array, output of SP/TM
  SDR outSPlocal(spLocal.getColumnDimensions()); //for SPlocal
  SDR outSP(vector<UInt>{COLS});
  SDR outTM(spGlobal.getColumnDimensions()); 
  Real an = 0.0f, anLikely = 0.0f; //for anomaly:
  MovingAverage avgAnom10(1000); //chose the window large enough so there's (some) periodicity in the patter, so TM can learn something

  //metrics
  Metrics statsInput(input, 1000);
  Metrics statsSPlocal(outSPlocal, 1000);
  Metrics statsSPglobal(outSPglobal, 1000);
  Metrics statsTM(outTM, 1000);

  //uses fixed seed for deterministic output checks:
  Random rnd(42);
  spGlobal.setSeed(1);
  spLocal.setSeed(1);
  tm.setSeed(42);

  /*
   * For example: fn = sin(x) -> periodic >= 2Pi ~ 6.3 && x+=0.01 -> 630 steps to 1st period -> window >= 630
   */
  Real avgAnomOld_ = 1.0;
  NTA_CHECK(avgAnomOld_ >= avgAnom10.getCurrentAvg()) << "TM should learn and avg anomalies improve, but we got: "
    << avgAnomOld_ << " and now: " << avgAnom10.getCurrentAvg(); //invariant


  // Start a stopwatch timer
  std::cout << "starting:  " << to_string(EPOCHS) << " iterations.\n";
  tAll.start();

  //run
  float x=0.0f;
  for (UInt e = 0; e < EPOCHS; e++) { //FIXME EPOCHS is actually steps, there's just 1 pass through data/epoch.

    //Encode
    {
    tEnc.start();
    x+=0.01f; //step size for fn(x)
    enc.encode(sin(x), input); //model sin(x) function //TODO replace with CSV data
//    cout << x << "\n" << sin(x) << "\n" << input << "\n\n";
    tEnc.stop();

    tRng.start();
    input.addNoise(0.01f, rnd); //change 1% of the SDR for each iteration, this makes a random sequence, but seemingly stable
    tRng.stop();
    }

    //SP (global and local)
    if(useSPlocal) {
      tSPloc.start();
      spLocal.compute(input, true, outSPlocal);
      tSPloc.stop();

      outSP = outSPlocal;
    }

    if(useSPglobal) {
      tSPglob.start();
      spGlobal.compute(input, true, outSPglobal);
      tSPglob.stop();
    
      outSP = outSPglobal;
    }

    // TM
    if(useTM) {
      tTM.start();
      tm.compute(outSP, true /*learn*/); //to uses output of SPglobal
      tm.activateDendrites(); //required to enable tm.getPredictiveCells()
      tTM.stop();

      outTM = tm.cellsToColumns( tm.getPredictiveCells() );
    }


    //Anomaly (pure x likelihood)
    {
    an = tm.anomaly;
    avgAnom10.compute(an); //moving average
    if(e % 1000 == 0) {
      NTA_CHECK(avgAnomOld_ >= avgAnom10.getCurrentAvg()) << "TM should learn and avg anomalies improve, but we got: "
        << avgAnomOld_ << " and now: " << avgAnom10.getCurrentAvg(); //invariant
      avgAnomOld_ = avgAnom10.getCurrentAvg(); //update
    }
    tAnLikelihood.start();
    anLikely = anLikelihood.anomalyProbability(an); 
    tAnLikelihood.stop();
    }


    // print
    if (e == EPOCHS - 1) {
      tAll.stop();

      //print connections stats
      std::cout << "\nInput :\n" << statsInput
	   << "\nSP(local) " << spLocal.connections
	   << "\nSP(local) " << statsSPlocal
           << "\nSP(global) " << spGlobal.connections
	   << "\nSP(global) " << statsSPglobal
           << "\nTM " << tm.connections 
	   << "\nTM " << statsTM
	   << "\n";

      // output values
      std::cout << "Epoch = " << e+1 << std::endl;
      std::cout << "Anomaly = " << an << std::endl;
      std::cout << "Anomaly (avg) = " << avgAnom10.getCurrentAvg() << std::endl;
      std::cout << "Anomaly (Likelihood) = " << anLikely << std::endl;
      std::cout << "input = " << input << std::endl;
      if(useSPlocal) std::cout << "SP (g)= " << outSP << std::endl;
      if(useSPlocal) std::cout << "SP (l)= " << outSPlocal <<std::endl;
      if(useTM) std::cout << "TM= " << outTM << std::endl;

      //timers
      std::cout << "==============TIMERS============" << std::endl;
      std::cout << "Init:\t" << tInit.getElapsed() << std::endl;
      std::cout << "Random:\t" << tRng.getElapsed() << std::endl;
      std::cout << "Encode:\t" << tEnc.getElapsed() << std::endl;
      if(useSPlocal)  std::cout << "SP (l):\t" << tSPloc.getElapsed()*1.0f  << std::endl;
      if(useSPglobal) std::cout << "SP (g):\t" << tSPglob.getElapsed() << std::endl;
      if(useTM) std::cout << "TM:\t" << tTM.getElapsed() << std::endl;
      std::cout << "AN:\t" << tAnLikelihood.getElapsed() << std::endl;

      // check deterministic SP, TM output 
      SDR goldEnc({DIM_INPUT});
      const SDR_sparse_t deterministicEnc{
        0, 4, 13, 21, 24, 30, 32, 37, 40, 46, 47, 48, 50, 51, 64, 68, 79, 81, 89, 97, 99, 114, 120, 135, 136, 140, 141, 143, 144, 147, 151, 155, 161, 162, 164, 165, 169, 172, 174, 179, 181, 192, 201, 204, 205, 210, 213, 226, 227, 237, 242, 247, 249, 254, 255, 262, 268, 271, 282, 283, 295, 302, 306, 307, 317, 330, 349, 353, 366, 380, 383, 393, 404, 409, 410, 420, 422, 441,446, 447, 456, 458, 464, 468, 476, 497, 499, 512, 521, 528, 531, 534, 538, 539, 541, 545, 550, 557, 562, 565, 575, 581, 589, 592, 599, 613, 617, 622, 647, 652, 686, 687, 691, 699, 704, 710, 713, 716, 722, 729, 736, 740, 747, 749, 753, 754, 758, 766, 778, 790, 791, 797, 800, 808, 809, 812, 815, 826, 828, 830, 837, 852, 853, 856, 863, 864, 873, 878, 882, 885, 893, 894, 895, 905, 906, 914, 915, 920, 924, 927, 937, 939, 944, 947, 951, 954, 956, 967, 968, 969, 973, 975, 976, 979, 981, 991, 998
      };
      goldEnc.setSparse(deterministicEnc);

      SDR goldSP({COLS});
      const SDR_sparse_t deterministicSP{
        68, 79, 86, 98, 105, 257, 263, 286, 302, 306, 307, 309, 310, 313, 315, 317, 318, 320, 323, 325, 326, 329, 334, 350, 356, 363, 539, 935, 1089, 1093, 1098, 1111, 1112, 1118, 1120, 1124, 1133, 1508, 1513, 1521, 1624, 1746, 1765, 1774, 1775, 1776, 1780, 1784, 1787, 1802, 1804, 1811, 1813, 1815, 1819, 1844, 1845, 1865, 1876, 1884, 1891, 1900, 1903, 1904, 1908, 1909, 1925, 1926, 1928, 1932, 1933, 1943, 1947, 1952, 1955, 1959, 1961, 1962, 1964, 1966, 1967, 1969, 1970, 1971, 1973, 1975, 1977, 1980, 1981, 1982, 1983, 1985, 1987, 1991, 1994, 2002, 2004, 2011, 2027, 2030, 2031, 2045
      };
      goldSP.setSparse(deterministicSP);

      SDR goldSPlocal({COLS});
      const SDR_sparse_t deterministicSPlocal{
        17, 71, 75, 79, 81, 86, 89, 164, 189, 198, 203, 262, 297, 314, 324, 326, 329, 337, 360, 379, 432, 443, 448, 452, 509, 520, 525, 526, 529, 536, 612, 619, 624, 630, 649, 652, 693, 717, 719, 720, 754, 810, 813, 815, 835, 839, 849, 884, 890, 914, 925, 931, 937, 945, 971, 1016, 1088, 1089, 1095, 1105, 1109, 1133, 1159, 1209, 1214, 1228, 1235, 1241, 1244, 1273, 1295, 1314, 1329, 1336, 1342, 1427, 1435, 1436, 1448, 1461, 1486, 1496, 1500, 1523, 1561, 1572, 1576, 1603, 1610, 1624, 1635, 1649, 1664, 1685, 1725, 1732, 1741, 1758, 1800, 1804, 1811, 1824, 1862, 1870, 1882, 1883, 1887, 1903, 1956, 1963, 1971, 1977, 1984, 2015
      };
      goldSPlocal.setSparse(deterministicSPlocal);

      SDR goldTM({COLS});
      const SDR_sparse_t deterministicTM{
        79, 92, 98, 128, 136, 286, 307, 309, 310, 313, 315, 325, 356, 454, 539, 1093, 1111, 1112, 1120, 1237, 1278, 1467, 1497, 1508, 1513, 1521, 1614, 1624, 1635, 1668, 1669, 1673,1699, 1774, 1775, 1776, 1780, 1784, 1808, 1813, 1815, 1816, 1821, 1827, 1845, 1865, 1900, 1911, 1913, 1925, 1929, 1931, 1932, 1933, 1940, 1941, 1947, 1949, 1955, 1956, 1959, 1961, 1962, 1964, 1966, 1967, 1968, 1969, 1970, 1972, 1975, 1977, 1978, 1979, 1981, 1982, 1985, 1987, 1988, 1991, 1994, 2027, 2030, 2044, 2045
      };
      goldTM.setSparse(deterministicTM);

      const float goldAn    = 0.470588f; //Note: this value is for a (randomly picked) datapoint, it does not have to improve (decrease) with better algorithms
      const float goldAnAvg = 0.40961f; // ...the averaged value, on the other hand, should improve/decrease. 

#ifdef _ARCH_DETERMINISTIC
      if(e+1 == 5000) {
	// For debugging serialization: save SP's state in 1 step, comment out, recompile, load SP and compare in another 
	// step 1:
	spGlobal.saveToFile("/tmp/spG.save");
	// step 2:
	SpatialPooler resumedSP;
	resumedSP.loadFromFile("/tmp/spG.save");
	NTA_CHECK(spGlobal == resumedSP) << "SPs differ!";
	// --end of debugging

        //these hand-written values are only valid for EPOCHS = 5000 (default), but not for debug and custom runs.
        NTA_CHECK(input == goldEnc) << "Deterministic output of Encoder failed!\n" << input << "should be:\n" << goldEnc;
        if(useSPglobal) { NTA_CHECK(outSPglobal == goldSP) << "Deterministic output of SP (g) failed!\n" << outSP << "should be:\n" << goldSP; }
        if(useSPlocal) {  NTA_CHECK(outSPlocal == goldSPlocal) << "Deterministic output of SP (l) failed!\n" << outSPlocal << "should be:\n" << goldSPlocal; }
        if(useTM) {       NTA_CHECK(outTM == goldTM) << "Deterministic output of TM failed!\n" << outTM << "should be:\n" << goldTM; }
        NTA_CHECK(static_cast<UInt>(an *10000.0f) == static_cast<UInt>(goldAn *10000.0f)) //compare to 4 decimal places
                  << "Deterministic output of Anomaly failed! " << an << "should be: " << goldAn;
        NTA_CHECK(static_cast<UInt>(avgAnom10.getCurrentAvg() * 10000.0f) == static_cast<UInt>(goldAnAvg * 10000.0f))
                  << "Deterministic average anom score failed:" << avgAnom10.getCurrentAvg() << " should be: " << goldAnAvg;
        std::cout << "outputs match\n";
      }
#endif

      // check runtime speed
      const size_t timeTotal = (size_t)floor(tAll.getElapsed());
      std::cout << "Total elapsed time = " << timeTotal << " seconds" << std::endl;
      if(EPOCHS >= 100) { //show only relevant values, ie don't run in valgrind (ndebug, epochs=5) run
#ifdef NTA_OS_LINUX
        const size_t CI_avg_time = (size_t)floor(99*Timer::getSpeed()); //sec //FIXME the CI speed broken for docker linux
        NTA_CHECK(timeTotal <= CI_avg_time) << //we'll see how stable the time result in CI is, if usable
          "HelloSPTP test slower than expected! (" << timeTotal << ",should be "<< CI_avg_time << "), speed coef.= " << Timer::getSpeed();
#endif
      }
    }
  } //end for
  return tAll.getElapsed();
} //end run()
} //-ns
