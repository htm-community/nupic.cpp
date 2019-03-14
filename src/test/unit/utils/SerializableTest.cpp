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

#include <nupic/types/Types.hpp>
#include <nupic/types/Serializable.hpp>
#include <nupic/utils/StlIo.hpp>
#include <nupic/utils/Random.hpp>
#include <sstream>

using namespace nupic;



TEST(SerializableTest, string_serialize)
{
  std::stringstream ss;
  std::string orig = " This\\\" \"is a \\string ";
  std::string result;
  std::string expected = "\" This\\\\\\\" \\\"is a \\\\string \"";
  stringOut(ss, orig);
  result = ss.str();
  EXPECT_TRUE(result == expected);

  ss.seekp(0);
  stringIn(ss, result);
  EXPECT_TRUE(result == orig);
}



TEST(SerializableTest, shared_ptr_serialize) 
{
  // We pick on Random because its an easy class to use.
  std::shared_ptr<Random> sp1, sp2;
  Random *rnd = new Random(42);
  sp1.reset(rnd);

  std::stringstream ss;
  ss << sp1 << std::endl;
	
  std::string expected = "random-v2 42 0 endrandom-v2 \n";
	EXPECT_TRUE(ss.str() == expected);
	
	ss.seekp(0); // rewind
  ss >> sp2;
  EXPECT_TRUE(*sp1.get() == *sp2.get());
}

TEST(SerializableTest, vector_serialize) 
{
  const std::vector<UInt> v = {1,2,3};
  std::stringstream ss;
   ss << "Vector " << v << std::endl;
	
  std::string expected = "Vector [3| 1 2 3 ]\n";
	EXPECT_TRUE(ss.str() == expected);
	
	ss.seekp(0); // rewind
  std::vector<UInt> v2;
  std::string tag;
  ss >> tag;  // "Vector"
  ss >> v2;
  EXPECT_TRUE(v == v2);
}


class Dummy : public Serializable {
  public:	
  int i = 3;
  std::vector<int> v = {1, 2, 3};
	std::map<std::string, int> m = {
				{"first",  1}, 
				{"second", 2}, 
				{"third",  3}
	  };
	std::map<std::string, std::vector<int>> mv = {
				{"first",  { 1, 1, 15}}, 
				{"second", { 2, 2, 25}}, 
				{"third",  { 3, 3, 35}}
	  };

  //Serializable interface
  void save(std::ostream &o) const override {
    o << "DUMMY ";
    o << i << " ";
    o << v << " ";
		//o << m << " ";
		//o << mv << " ";
    o << "~DUMMY";
  }

  void load(std::istream &in) override {
    std::string label;
    in >> label;
    ASSERT_EQ(label, "DUMMY");
    in >> i;
		in >> v;
		//in >> m;
		//in >> mv;
    in >> label;
    ASSERT_EQ(label, "~DUMMY");
  }

};
// serialization/deserialization
std::ostream &operator<<(std::ostream &f, const Dummy &demo) {
  //f << demo.i << " " << demo.v << " " << demo.m << " " demo.mv << std::endl;
  f << demo.i << " " << demo.v << std::endl;
  return f;
}
std::istream &operator>>(std::istream &f, Dummy &demo) {
  //f >> demo.i >> demo.v >> demo.m >> demo.mv;
  f >> demo.i >> demo.v;
  return f;
}

TEST(SerializableTest, demo)
{
  Dummy demo1;
  std::stringstream ss;
  demo1.save(ss);
  Dummy demo2;
  demo2.i = 5; //manualy change, to have a difference
  demo2.v = {8, 9, 10, 234};
  demo2.load(ss);
  ASSERT_EQ(demo1.i, demo2.i);
  ASSERT_EQ(demo1.v, demo2.v);
  //ASSERT_EQ(demo1.m, demo2.m);

  Dummy demo3;
  ss.seekg(0);
  ss << demo3;
  ss.flush();

  Dummy demo4;
  ss >> demo4;
  ASSERT_EQ(demo3.i, demo4.i);
  ASSERT_EQ(demo3.v, demo4.v);
  //ASSERT_EQ(demo3.m, demo4.m);
}

