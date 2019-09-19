/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2013, Numenta, Inc.
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

/** @file
 * Implementation of YAML tests
 */

#include "gtest/gtest.h"
#include <htm/engine/Network.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/ntypes/ValueMap.hpp>

namespace testing { 
    
using namespace htm;

TEST(ValueMapTest, IntScalerTest) {
  ValueMap vm;

  const char *s1 = "10";
  vm.parse(s1);
  EXPECT_TRUE(vm.isScalar());
  UInt32 u = vm.as<UInt32>();
  EXPECT_EQ(10u, u);

  vm.parse("-1"); 
  EXPECT_ANY_THROW(UInt32 x = vm.as<UInt32>());  // Unsigned must be positive.
  Int32 i = vm.as<Int32>();
  EXPECT_EQ(-1, i);

  vm.parse("- 1");
  EXPECT_TRUE(vm.isSequence());
  u = vm[0].as<UInt32>();
  EXPECT_EQ(1, u);

  vm.parse("[123]");   // explicit sequence with one element.
  EXPECT_TRUE(vm.isSequence());
  i = vm[0].as<Int32>();
  EXPECT_EQ(123, i);

  EXPECT_ANY_THROW(Int32 i = vm.parse("999999999999999999999999999").as<Int32>());
  EXPECT_ANY_THROW(Int32 i = vm.parse("-1").as<UInt32>());
  EXPECT_ANY_THROW(Int32 i = vm.parse("abc").as<Int32>());
  EXPECT_ANY_THROW(Int32 i = vm.parse("").as<Int32>());
}

TEST(ValueMapTest, toValueTestReal32) {
  ValueMap vm;
  vm.parse("10.1");
  Real32 x = vm.getScalarT<Real32>("");
  EXPECT_NEAR(10.1f, x, 0.000001);
}

TEST(ValueMapTest, toValueTestString) {
  ValueMap vm;
  std::string s1 = "\"this is a string\"";
  vm.parse(s1);
  std::string s = vm.getScalarT<std::string>("");
  EXPECT_TRUE(s == s1);

  std::string s2 = "this is a string";
  vm.parse(s1);
  s = vm.getScalarT<std::string>("");
  EXPECT_TRUE(s == s2);

  s = vm.str();
  EXPECT_TRUE(s == s2);
}

TEST(ValueMapTest, toValueTestBool) {
  ValueMap vm;
  EXPECT_TRUE(vm.parse("true").getScalarT<bool>(""));
  EXPECT_TRUE(vm.parse("1").getScalarT<bool>(""));
  EXPECT_FALSE(vm.parse("false").getScalarT<bool>(""));
  EXPECT_FALSE(vm.parse("0").getScalarT<bool>(""));
  EXPECT_ANY_THROW(vm.parse("1234").getScalarT<bool>(""));
}

TEST(ValueMapTest, asArray) {
  ValueMap vm;
  std::string json = "[1,2,3,4,5]";
  vm.parse(json);

  EXPECT_EQ(ValueMap::Sequence, vm.getCategory());
  EXPECT_TRUE(vm.isSequence());
  ASSERT_TRUE(!vm.isScalar());
  ASSERT_TRUE(!vm.isScalar());
  ASSERT_TRUE(!vm.isEmpty());

  std::vector<Int32> s1 = vm.asVector<Int32>();
  std::vector<Int32> s2 = {1, 2, 3, 4, 5};
  ASSERT_TRUE(s1 == s2);

  ASSERT_EQ(vm[0].as<Int32>(), 1);
  ASSERT_EQ(vm[0].str(), "1");

  ASSERT_ANY_THROW(vm.as<Int32>()); // not a scaler
  ASSERT_ANY_THROW(vm["foobar"]);    // not a map
}


} // namespace testing