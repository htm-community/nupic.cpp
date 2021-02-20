/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2020, Numenta, Inc.
 *
 * Author:  Zbysek Zapadlik, 07/07/2020  zbysekzapadlik@seznam.cz
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

/*
 * This test is napi_hello example stripped down to bare bones
 * */

#include <htm/utils/Log.hpp>
#include "sqlite3.h"
#include "gtest/gtest.h"

#include <htm/engine/Network.hpp>
#include <htm/utils/Log.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/regions/DatabaseRegion.hpp>

namespace testing {

using namespace htm;


TEST(DatabaseRegionTest, overall)
{
	htm::UInt EPOCHS = 20;      // number of iterations (calls to encoder/SP/TP compute() )

	const UInt DIM_INPUT = 1000;  // Width of encoder output
	const UInt COLS = 2048;       // number of columns in SP, TP
	const UInt CELLS = 8;         // cells per column in TP


	std::string encoder_params   = "{size: " + std::to_string(DIM_INPUT) + ", sparsity: 0.2, radius: 0.03, seed: 2019, noise: 0.01}";
	std::string sp_global_params = "{columnCount: " + std::to_string(COLS) + ", globalInhibition: true}";
	std::string tm_params        = "{cellsPerColumn: " + std::to_string(CELLS) + ", orColumnOutputs: true}";


	std::string output_file = ":memory:";// in memory for this unit test. or could be physical file like: NapiOutputDir/Output.db

	try {

		Network net;

		// Declare the regions to use
		std::shared_ptr<Region> encoder   = net.addRegion("encoder",   "RDSEEncoderRegion", encoder_params);
		std::shared_ptr<Region> sp_global = net.addRegion("sp_global", "SPRegion",   sp_global_params);
		std::shared_ptr<Region> tm        = net.addRegion("tm",        "TMRegion",   tm_params);
		std::shared_ptr<Region> output 	  = net.addRegion("output",    "DatabaseRegion", "{outputFile: '"+ output_file + "'}");

		// Setup data flows between regions
		net.link("encoder",   "sp_global", "", "", "encoded", "bottomUpIn");
		net.link("sp_global", "tm",        "", "", "bottomUpOut", "bottomUpIn");
		net.link("tm", "output", "", "", "anomaly", "dataIn0");
		net.link("encoder", "output", "", "", "bucket", "dataIn1");


		net.initialize();

		// RUN
		float x = 0.00f;
		for (size_t e = 0; e < EPOCHS; e++) {
			// genarate some data to send to the encoder

			// -- sine wave, 0.01 radians per iteration   (Note: first iteration is for x=0.01, not 0)
			x += 0.01f; // step size for fn(x)
			double data = std::sin(x);
			encoder->setParameterReal64("sensedValue", data); // feed data into RDSE encoder for this iteration.

			// Execute an iteration.
			net.run(1);
		}

		output->executeCommand({ "commitTransaction" });//commits sql queries since now (this is performance speedup)
		//now gets number of rows in the database
		UInt rowCount = std::stoi(output->executeCommand({ "getRowCount" }));
		//there should be EPOCHS*2 rows in whole database - we are storing two values into two tables each iteration
		EXPECT_EQ(rowCount, EPOCHS*2);

		// close database file
		output->executeCommand({ "closeFile" });


	 } catch (Exception &ex) {
          // close database file
          FAIL() << "Catched exception : " << ex.getMessage();
	 }
}


TEST(DatabaseRegionTest, getSpecJSON) {
  std::string expected = R"({"spec": "DatabaseRegion",
  "description": "DatabaseRegion is a node that writes multiple scalar streams to a SQLite3 database file (.db). The target filename is specified using the 'outputFile' parameter at run time. On each compute, all inputs are written to the database.",
  "parameters": {
    "outputFile": {
      "description": "Writes data stream to this database file on each compute. Database is recreated on initialization This parameter must be set at runtime before the first compute is called. Throws an exception if it is not set or the file cannot be written to.",
      "type": "String",
      "count": 1,
      "access": "ReadWrite",
      "defaultValue": ""
    }
  },
  "commands": {
    "closeFile": "Close the current database file, if open.",
    "getRowCount": "Gets sum of row counts for all tables in opened database.",
    "commitTransaction": "Commits currently active transaction. Speeding up write avoiding repeat writes in loop.Transaction is started when databse is opened."
    },
  "inputs": {
    "dataIn0": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn1": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn2": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn3": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn4": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn5": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn6": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn7": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn8": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    },
    "dataIn9": {
      "description": "Data scalar to be written to the database",
      "type": "Real32",
      "count": 0,
      "required": 0,
      "regionLevel": 1,
      "isDefaultInput": 1
    }
  },
  "outputs": {
  }
})";

  Spec *spec = DatabaseRegion::createSpec();
  std::string json = spec->toString();
  EXPECT_STREQ(json.c_str(), expected.c_str());
} // namespace testing

TEST(DatabaseRegionTest, getParameters) {
  std::string expected = "{\n  \"outputFile\": \":memory:\"\n}";
  Network net1;
  std::string output_file = ":memory:"; // in memory for this unit test. or could be physical file like: NapiOutputDir/Output.db
  std::shared_ptr<Region> region1 = net1.addRegion("db", "DatabaseRegion", "{outputFile: '" + output_file + "'}");
  std::string json = region1->getParameters();
  EXPECT_STREQ(json.c_str(), expected.c_str());
}
} // testing namespace
