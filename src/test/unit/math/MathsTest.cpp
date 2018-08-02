/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
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
 *
 * http://numenta.org/licenses/
 * ---------------------------------------------------------------------
 */

/** @file
 * Implementation for maths unit tests
 */

#include <nupic/math/utils.hpp>
#include <nupic/math/ArrayAlgo.hpp>
#include <nupic/utils/TRandom.hpp>

#include <nupic/math/Math.hpp>
#include "gtest/gtest.h"


using namespace std;

namespace nupic {


//--------------------------------------------------------------------------------
  TEST(MathsTest, unitTestNearlyZero)
  {
    EXPECT_TRUE(nearlyZero(Real(0.0000000001))) << "nearlyZero Reals 1";
    EXPECT_TRUE(nearlyZero(0.0)) << "nearlyZero Reals 2";
    EXPECT_FALSE(nearlyZero(1.0)) << "nearlyZero Reals 3";
    EXPECT_FALSE(nearlyZero(0.01)) << "nearlyZero Reals 4";
    EXPECT_FALSE(nearlyZero(-0.01)) << "nearlyZero Reals 5";
    EXPECT_FALSE(nearlyZero(-1.0)) << "nearlyZero Reals 6";
    EXPECT_FALSE(nearlyZero(2.0)) << "nearlyZero Reals 7";
    EXPECT_FALSE(nearlyZero(1.99999999)) << "nearlyZero Reals 8";
    EXPECT_FALSE(nearlyZero(-2.00000001)) << "nearlyZero Reals 9";
    EXPECT_FALSE(nearlyZero(-1.99999999)) << "nearlyZero Reals 10";
  }

  //--------------------------------------------------------------------------------
  TEST(MathsTest,unitTestNearlyEqual)
  {
    EXPECT_TRUE(nearlyEqual(Real(0.0), Real(0.0000000001))) << "nearlyEqual Reals 1";
    EXPECT_TRUE(nearlyEqual(0.0, 0.0)) << "nearlyEqual Reals 2";
    EXPECT_FALSE(nearlyEqual(0.0, 1.0)) << "nearlyEqual Reals 3";
    EXPECT_FALSE(nearlyEqual(0.0, 0.01)) << "nearlyEqual Reals 4";;
    EXPECT_FALSE(nearlyEqual(0.0, -0.01)) << "nearlyEqual Reals 5";;
    EXPECT_FALSE(nearlyEqual(0.0, -1.0)) << "nearlyEqual Reals 6";;
    EXPECT_TRUE(nearlyEqual(Real(2.0), Real(2.000000001))) << "nearlyEqual Reals 7";
    EXPECT_TRUE(nearlyEqual(Real(2.0), Real(1.999999999))) << "nearlyEqual Reals 8";
    EXPECT_TRUE(nearlyEqual(Real(-2.0), Real(-2.000000001))) << "nearlyEqual Reals 9";
    EXPECT_TRUE(nearlyEqual(Real(-2.0), Real(-1.999999999))) << "nearlyEqual Reals 10";
  }

  //--------------------------------------------------------------------------------
  TEST(MathsTest, unitTestNearlyEqualVector)
  { 
    vector<Real> v1, v2;
    
    {
      EXPECT_TRUE(nearlyEqualVector(v1, v2)) << "empty vectors";
      
      v2.push_back(1);
      EXPECT_FALSE(nearlyEqualVector(v1, v2)) << " different sizes";
      
      v1.push_back(1);
      EXPECT_TRUE(nearlyEqualVector(v1, v2)) << " 1 element";
      
//#if 0
//      for(UInt i=0; i<2048; ++i) {
//        Real v = Real(rng_->get() % 256)/256.0;
//        v1.push_back(v);
//        v2.push_back(v);
//      }
//      EXPECT_TRUE(nearlyEqualVector(v1, v2));
//      
//      v2[512] += 1.0;
//      EXPECT_FALSE(nearlyEqualVector(v1, v2));
//      
//      v1.clear(); v2.clear();
//      EXPECT_TRUE(nearlyEqualVector(v1, v2));
//#endif
    }
  }

  //--------------------------------------------------------------------------------
  TEST(MathsTest,unitTestNormalize)
  {
    {
      vector<Real> v1(3), answer(3);

      {
        vector<Real> empty1;
        normalize(empty1.begin(), empty1.end());
        EXPECT_TRUE(nearlyEqualVector(empty1, empty1)) << "Normalize vector<Real>, empty";
      }
	  
      {
        v1[0] = Real(0.0); v1[1] = Real(0.0); v1[2] = Real(0.0);
        answer[0] = Real(0.0); answer[1] = Real(0.0); answer[2] = Real(0.0);
        normalize(v1.begin(), v1.end());
        EXPECT_TRUE(nearlyZero(sum(v1))) << "Normalize vector<Real>, 11";
        EXPECT_TRUE(nearlyEqualVector(v1, answer)) << "Normalize vector<Real>, 12";
      }

      {
        v1[0] = Real(1.0); v1[1] = Real(1.0); v1[2] = Real(1.0);
        answer[0] = Real(1.0/3.0); answer[1] = Real(1.0/3.0); answer[2] = Real(1.0/3.0);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0))) << "Normalize vector<Real>, 21";
        EXPECT_TRUE(nearlyEqualVector(v1, answer)) << "Normalize vector<Real>, 22";
      }

      {
        v1[0] = Real(0.5); v1[1] = Real(0.5); v1[2] = Real(0.5);
        answer[0] = Real(0.5/1.5); answer[1] = Real(0.5/1.5); answer[2] = Real(0.5/1.5);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0))) << "Normalize vector<Real>, 31";
        EXPECT_TRUE(nearlyEqualVector(v1, answer)) << "Normalize vector<Real>, 32";
      }

      {
        v1[0] = Real(1.0); v1[1] = Real(0.5); v1[2] = Real(1.0);
        answer[0] = Real(1.0/2.5); answer[1] = Real(0.5/2.5); answer[2] = Real(1.0/2.5);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0))) << "Normalize vector<Real>, 41";
        EXPECT_TRUE(nearlyEqualVector(v1, answer)) << "Normalize vector<Real>, 42";
      }

      { // Test normalizing to non-1.0
        v1[0] = Real(1.0); v1[1] = Real(0.5); v1[2] = Real(1.0);
        answer[0] = Real(3.0/2.5); answer[1] = Real(1.5/2.5); answer[2] = Real(3.0/2.5);
        normalize(v1.begin(), v1.end(), 1.0, 3.0);
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(3.0))) << "Normalize vector<Real>, 51";
        EXPECT_TRUE(nearlyEqualVector(v1, answer)) << "Normalize vector<Real>, 52";
      }
    }
	
    { // normalize
      std::vector<Real> v1(3), answer(3);

      {
        std::vector<Real> empty1;
        normalize(empty1.begin(), empty1.end());
        EXPECT_TRUE(nearlyEqualVector(empty1, empty1));
      }

      {
        v1[0] = 0.0; v1[1] = 0.0; v1[2] = 0.0;
        answer[0] = 0.0; answer[1] = 0.0; answer[2] = 0.0;
        normalize(v1.begin(), v1.end());
        EXPECT_TRUE(nearlyZero(sum(v1)));
        EXPECT_TRUE(nearlyEqualVector(v1, answer));
      }

      {
        v1[0] = Real(1.0); v1[1] = Real(1.0); v1[2] = Real(1.0);
        answer[0] = Real(1.0/3.0); answer[1] = Real(1.0/3.0); answer[2] = Real(1.0/3.0);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0)));
        EXPECT_TRUE(nearlyEqualVector(v1, answer));
      }

      {
        v1[0] = Real(0.5); v1[1] = Real(0.5); v1[2] = Real(0.5);
        answer[0] = Real(0.5/1.5); answer[1] = Real(0.5/1.5); answer[2] = Real(0.5/1.5);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0)));
        EXPECT_TRUE(nearlyEqualVector(v1, answer));
      }

      {
        v1[0] = Real(1.0); v1[1] = Real(0.5); v1[2] = Real(1.0);
        answer[0] = Real(1.0/2.5); answer[1] = Real(0.5/2.5); answer[2] = Real(1.0/2.5);
        normalize(v1.begin(), v1.end());
        Real s = sum(v1);
        EXPECT_TRUE(nearlyEqual(s, Real(1.0)));
        EXPECT_TRUE(nearlyEqualVector(v1, answer));
      }
    }
  }



  //--------------------------------------------------------------------------------
  TEST(MathsTest,unitTestWinnerTakesAll)
  {
    UInt nchildren, ncols, w, nreps;

    nchildren = 21;
    w = 125;
    nreps = 1000;
    TRandom rng_("maths_test");
    
    vector<UInt> boundaries(nchildren, 0);
    
    boundaries[0] = rng_.getUInt32(w) + 1;
    for (UInt i = 1; i < (nchildren-1); ++i) 
      boundaries[i] = boundaries[i-1] + (rng_.getUInt32(w) + 1);
    ncols = nchildren * w;
    boundaries[nchildren-1] = ncols;

    for (UInt i = 0; i < nreps; ++i) {
      vector<Real64> x(ncols), v(ncols);
      for (UInt j = 0; j < ncols; ++j)
        x[j] = rng_.getReal64();

      winnerTakesAll2(boundaries, x.begin(), v.begin());
      
      UInt k2 = 0;
      for (UInt k1 = 0; k1 < nchildren; ++k1) {

        vector<Real64>::iterator it = 
          max_element(x.begin() + k2, x.begin() + boundaries[k1]);
        EXPECT_EQ(v[it - x.begin()], 1) << "Maths winnerTakesAll2 1";

        Real64 s = accumulate(v.begin() + k2, v.begin() + boundaries[k1], (Real64)0);
        EXPECT_EQ(s, (Real64)1) << "Maths winnerTakesAll2 2";
        
        k2 = boundaries[k1];
      }

      Real64 s = accumulate(v.begin(), v.end(), (Real64)0);
      EXPECT_EQ( s, (Real64) nchildren) << "Maths winnerTakesAll2 3";
    }
  }

  //--------------------------------------------------------------------------------
  TEST(MathsTest,unitTestScale)
  {
    {
      vector<Real> x;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(x.empty()) << "scale 1";
    }

    {
      vector<Real> x(1);
      x[0] = 1;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(nearlyEqual(x[0], (Real)1)) << "scale 2";
      
      x[0] = 2;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(nearlyEqual(x[0], (Real)1)) << "scale 3";
      
      normalize_max(x.begin(), x.end(), .5);
      EXPECT_TRUE(nearlyEqual(x[0], (Real).5)) << "scale 4";

      // TODO: test negative values in x
    }

    {
      vector<Real> x(2);
      x[0] = 1; x[1] = .5;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(nearlyEqual(x[0], (Real)1)) << "scale 5a";
      EXPECT_TRUE(nearlyEqual(x[1], (Real).5)) << "scale 5b";

      x[0] = 10; x[1] = 7;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(nearlyEqual(x[0], (Real)1)) << "scale 6a";
      EXPECT_TRUE(nearlyEqual(x[1], (Real).7)) << "scale 6b";

      x[0] = 7; x[1] = 10;
      normalize_max(x.begin(), x.end(), 1);
      EXPECT_TRUE(nearlyEqual(x[0], (Real).7)) << "scale 7a";
      EXPECT_TRUE(nearlyEqual(x[1], (Real)1)) << "scale 7b";

      normalize_max(x.begin(), x.end(), 10);
      EXPECT_TRUE(nearlyEqual(x[0], (Real)7)) << "scale 8a";
      EXPECT_TRUE(nearlyEqual(x[1], (Real)10)) << "scale 8b";
    }    

    {
      TRandom rng_("maths_test");
      const UInt N = 256;
      vector<Real64> x(N), ans(N);
      ITER_1(100) {
        for (UInt j = 0; j < N; ++j)
          x[j] = ans[j] = rng_.getReal64();

        Real64 max = 0;
        for (UInt j = 0; j < N; ++j)
          if (ans[j] > max)
            max = ans[j];
        for (UInt j = 0; j < N; ++j)
          ans[j] /= max;

        normalize_max(x.begin(), x.end(), 1);
        
        bool identical = true;
        for (UInt j = 0; j < N && identical; ++j)
          if (!nearlyEqual(x[j], ans[j]))
            identical = false;

        EXPECT_TRUE(identical) << "scale 9";
      }
    }
  }
} // end namespace nupic


