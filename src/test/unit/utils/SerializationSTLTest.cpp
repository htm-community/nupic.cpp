#include <gtest/gtest.h>

#include <nupic/types/Serializable.hpp>
#include <nupic/utils/StlIo.hpp>
#include <sstream>

#include <vector>

namespace testing {
using namespace std;
using namespace nupic;
class Dummy : public Serializable {
  public:	
  int i = 3;
  vector<int> v = vector<int>{1, 2, 3};

  //Serializable interface
  void save(ostream &o) const override {
    o << "DUMMY ";
    o << i << " ";
//!    o << v << " ";
    o << "~DUMMY";
  }

  void load(istream &in) override {
    string label;
    in >> label;
    ASSERT_EQ(label, "DUMMY");
    in >> i;
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
  demo2.v = {};
  demo2.load(ss);
  ASSERT_EQ(demo.i, demo2.i);
//!  ASSERT_EQ(demo.v, demo2.v);
}

TEST(SerializableSTLTest, stl_save) 
{
  //from StlIo.hpp
  const vector<int> v = vector<int>{1, 2, 3};
  stringstream ss;
  ss << v;
  vector<int> v2;
  ss >> v2;
  ASSERT_EQ(v, v2);
}

} //-ns
