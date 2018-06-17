/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2013-2017, Numenta, Inc.  Unless you have an agreement
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
 * Implementation of Input test
 */

#include <nupic/engine/Input.hpp>
#include <nupic/engine/Network.hpp>
#include <nupic/engine/Output.hpp>
#include <nupic/engine/Region.hpp>
#include <nupic/engine/TestNode.hpp>
#include "gtest/gtest.h"

using namespace nupic;

TEST(InputTest, BasicNetworkConstruction)
{
    Network net;
    auto r1 = net.addRegion("r1", "TestNode", "");
    auto r2 = net.addRegion("r2", "TestNode", "");

    //Test constructor
    Input x(*r1, NTA_BasicType_Int32);
    Input y(*r2, NTA_BasicType_Byte);
    EXPECT_THROW(Input i(*r1, (NTA_BasicType)(NTA_BasicType_Last + 1)),
        std::exception);

    //test getRegion()
    ASSERT_EQ(r1.get(), &(x.getRegion()));
    ASSERT_EQ(r2.get(), &(y.getRegion()));


    //test isInitialized()
    ASSERT_TRUE(!x.isInitialized());
    ASSERT_TRUE(!y.isInitialized());

    net.link("r1", "r2");

    net.initialize();

    Input *z = r1->getInput("bottomUpIn");

    //test getData()
    const Array * pa = &(z->getData());
    ASSERT_EQ(0u, pa->getCount());
    Real64* buf = (Real64*)(pa->getBuffer());
    ASSERT_TRUE(buf != nullptr);
    NTA_BasicType type = pa->getType();
    ASSERT_TRUE(type == NTA_BasicType_Real64);
}


TEST(InputTest, BufferManagement)
{
    Network net;
    auto region1 = net.addRegion("region1", "TestNode", "{count: 64}");
    auto region2 = net.addRegion("region2", "TestNode", "{count: 64}");


    //test addLink() indirectly - it is called by Network::link()
    net.link("region1", "region2");

    //test initialize(), which is called by net.initialize()
    net.initialize();

    Input * in1 = region1->getInput("bottomUpIn");
    Input * in2 = region2->getInput("bottomUpIn");
    Output * out1 = region1->getOutput("bottomUpOut");

    //test isInitialized()
    ASSERT_TRUE(in1->isInitialized());
    ASSERT_TRUE(in2->isInitialized());

    //test prepare
    {
        //set in2 to all zeroes
        const ArrayBase * ai2 = &(in2->getData());
        ASSERT_EQ(ai2->getCount(),64);
        Real64 *idata = (Real64 *)(ai2->getBuffer());
        for (UInt i = 0; i < 64; i++)
            idata[i] = 0;

        //set out1 to all 10's
        const ArrayBase *ao1 = &(out1->getData());
        ASSERT_EQ(ao1->getCount(),64);
        idata = (Real64 *)(ao1->getBuffer());
        for (UInt i = 0; i < 64; i++)
            idata[i] = 10;

        //confirm that in2 is still all zeroes
        ai2 = &(in2->getData());
        ASSERT_EQ(ai2->getCount(), 64);
        idata = (Real64 *)(ai2->getBuffer());
        //only test 4 instead of 64 to cut down on number of tests
        for (UInt i = 0; i < 4; i++)
            ASSERT_EQ(0, idata[i]);

        // moves Outputs to Inputs for region2.
        in2->prepare();

        //confirm that in2 is now all 10's
        ai2 = &(in2->getData());
        ASSERT_EQ(ai2->getCount(), 64);
        idata = (Real64 *)(ai2->getBuffer());
        //only test 4 instead of 64 to cut down on number of tests
        for (UInt i = 0; i < 4; i++)
            ASSERT_EQ(10, idata[i]);
    }

    net.run(2);



    //test getData()
    const ArrayBase * pa = &(in2->getData());
    ASSERT_EQ(64u, pa->getCount());
    Real64* data = (Real64*)(pa->getBuffer());
    ASSERT_EQ(1, data[0]);
    ASSERT_EQ(0, data[1]);
    ASSERT_EQ(29, data[30]);
    ASSERT_EQ(30, data[31]);
    ASSERT_EQ(62, data[63]);
}

TEST(InputTest, LinkTwoRegionsOneInput)
{
    Network net;
    auto region1 = net.addRegion("region1", "TestNode", "{count: 128}");
    auto region2 = net.addRegion("region2", "TestNode", "{count: 128}");
    auto region3 = net.addRegion("region3", "TestNode", "");


    net.link("region1", "region3", "", "");
    net.link("region2", "region3", "", "");

    net.initialize();

    net.run(2);


    //test getData()
    Input *in3 = region3->getInput("bottomUpIn");
    const ArrayBase *pa = &(in3->getData());
    ASSERT_EQ(256u, pa->getCount());  // double wide buffer
    Real64* data = (Real64*)(pa->getBuffer());
    ASSERT_EQ(1, data[0]);
    ASSERT_EQ(0, data[1]);
    ASSERT_EQ(29, data[30]);
    ASSERT_EQ(30, data[31]);
    ASSERT_EQ(62, data[63]);
    ASSERT_EQ(63, data[64]);
    ASSERT_EQ(64, data[65]);
    ASSERT_EQ(93, data[94]);
    ASSERT_EQ(94, data[95]);
    ASSERT_EQ(126, data[127]);

    ASSERT_EQ(1, data[128]);
    ASSERT_EQ(0, data[129]);
    ASSERT_EQ(126, data[255]);
}
