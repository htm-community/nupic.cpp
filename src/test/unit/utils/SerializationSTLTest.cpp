/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2016, Numenta, Inc.  Unless you have an agreement
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


#include <gtest/gtest.h>

#include <nupic/types/Serializable.hpp>
#include <nupic/utils/StlIo.hpp>
#include <sstream>


namespace testing {
using namespace nupic;


TEST(SerializableSTLTest, vector_save) 
{
  //from StlIo.hpp
  const std::vector<UInt> v = std::vector<UInt>({1, 2, 3});
  std::stringstream ss;
  ss << v;
	
	str::string expected = " ";
	EXPECT_EQ(ss.str(), expected);
	
	ss.seek(0); // rewind
  vector<int> v2;
  ss >> v2;
  EXPECT_EQ(v, v2);
}



class Dummy : public Serializable {
  public:	
  int i = 3;
  std::vector<int> v = std::vector<int>{1, 2, 3};
	std::map<std::string key, std::vector<int>> m = {
				{"first",  { 1, 1, 15}}, 
				{"second", { 2, 2, 25}}, 
				{"third",  { 3, 3, 35}}
	  };

  //Serializable interface
  void save(ostream &o) const override {
    o << "DUMMY ";
    o << i << " ";
    o << v << " ";
		o << m << " ";
    o << "~DUMMY";
  }

  void load(istream &in) override {
    string label;
    in >> label;
    ASSERT_EQ(label, "DUMMY");
    in >> i;
		in >> v;
		in >> m;
    in >> label;
    ASSERT_EQ(label, "~DUMMY");
  }
};

TEST(SerializableSTLTest, demo)
{
  Dummy demo;
  stringstream ss;
  demo.save(ss);
  Dummy demo2;
  demo2.i = 5; //manualy change, to have a difference
  demo2.v = {8, 9, 10, 234};
  demo2.load(ss);
  ASSERT_EQ(demo.i, demo2.i);
  ASSERT_EQ(demo.v, demo2.v);
  ASSERT_EQ(demo.m, demo2.m);
}

} //-ns
