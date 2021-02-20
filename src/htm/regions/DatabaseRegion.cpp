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
 * Implementation for DatabaseRegion class
 */

#include <iostream>
#include <list>
#include <sstream>
#include <stdexcept>
#include <string>
#include <stdio.h>

#include <htm/engine/Output.hpp>
#include <htm/engine/Input.hpp>
#include <htm/engine/Region.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/regions/DatabaseRegion.hpp>
#include <htm/utils/Log.hpp>


namespace htm {

static const UInt MAX_NUMBER_OF_INPUTS = 10;// maximal number of inputs/scalar streams in the database
static UInt auxRowCnt; // Auxiliary variable for getting row count from callback

static int SQLcallback(void *data, int argc, char **argv, char **azColName);


DatabaseRegion::DatabaseRegion(const ValueMap &params, Region* region)
    : RegionImpl(region), filename_(""),
	  dbHandle(nullptr),xTransactionActive(false) {
  if (params.contains("outputFile")) {
    std::string s = params.getString("outputFile", "");
    openFile(s);
  }
  else
    filename_ = "";

}

DatabaseRegion::DatabaseRegion(ArWrapper& wrapper, Region* region)
    : RegionImpl(region), filename_(""),
	  dbHandle(nullptr),xTransactionActive(false) {
  cereal_adapter_load(wrapper);
}


DatabaseRegion::~DatabaseRegion() { closeFile(); }

void DatabaseRegion::initialize() {
  NTA_CHECK(region_ != nullptr);
  // We have no outputs or parameters; just need our input.
  const std::map<std::string, std::shared_ptr<Input>> inputs = region_->getInputs();


  NTA_ASSERT(inputs.size()!=0) << "DatabaseRegion::initialize - no inputs configured\n";

  iTableCount = 0;
	for (const auto & inp : inputs) {
		const auto inObj = inp.second;
		if (inObj->hasIncomingLinks() && inObj->getData().getCount() != 0) { //create tables only for those, whose was configured
			createTable("dataStream_" + inp.first);
			iTableCount++;
		}
	}

}

void DatabaseRegion::createTable(const std::string &sTableName){

	/* Create SQL statement */
	std::string  sql = "CREATE TABLE "+sTableName+" (iteration INTEGER PRIMARY KEY, value REAL);";

	char *zErrMsg;
	/* Execute SQL statement */
	int returnCode = sqlite3_exec(dbHandle, sql.c_str(), nullptr, 0, &zErrMsg);

	if( returnCode != SQLITE_OK ){
		NTA_THROW << "Error creating SQL table, message:"
				  << zErrMsg;
		sqlite3_free(zErrMsg);
	}
}

void DatabaseRegion::insertData(const std::string &sTableName, const std::shared_ptr<Input> inputData){

	NTA_ASSERT(inputData->getData().getCount()==1);

	if(!xTransactionActive){
		//starts transaction, for speedup. Transaction ends by closing the file
		ExecuteSQLcommand("BEGIN TRANSACTION");
		xTransactionActive = true;
	}

	Real32* value = (Real32 *)inputData->getData().getBuffer();
	/* Create SQL statement */
	std::string  sql = "INSERT INTO "+sTableName+"(value) VALUES ("+std::to_string(*value) + ");";

	char *zErrMsg;

	int returnCode = sqlite3_exec(dbHandle, sql.c_str(), nullptr, 0, &zErrMsg);

	if( returnCode != SQLITE_OK ){
		NTA_THROW << "Error inserting data to SQL table, message:"
				  << zErrMsg;
		sqlite3_free(zErrMsg);
	}
}

void DatabaseRegion::compute() {


	const std::map<std::string, std::shared_ptr<Input>> inputs = region_->getInputs();


	NTA_ASSERT(inputs.size()!=0) << "DatabaseRegion::initialize - no inputs configured\n";

	for (const auto & inp : inputs)
	{
		const auto inObj = inp.second;
		if (inObj->hasIncomingLinks() && inObj->getData().getCount() != 0) { //create tables only for those, whose was configured
			insertData("dataStream_" + inp.first, inObj);
		}
	}
}

void DatabaseRegion::ExecuteSQLcommand(std::string sqlCommand){

	char *zErrMsg;
	int returnCode = sqlite3_exec(dbHandle, sqlCommand.c_str(), nullptr, 0, &zErrMsg);

	if( returnCode != SQLITE_OK ){
		NTA_THROW << "Error executing"
				<< sqlCommand
				<< ", message:"
				<< zErrMsg;
		sqlite3_free(zErrMsg);
	}
}

void DatabaseRegion::closeFile() {
  if (dbHandle!=NULL) {

  	if(xTransactionActive){
  		ExecuteSQLcommand("END TRANSACTION");//ends transaction. Now it flushes cache to the file for.
  		xTransactionActive=false;
  	}

		sqlite3_close(dbHandle);
		dbHandle = nullptr;
    filename_ = "";
  }
}

void DatabaseRegion::openFile(const std::string &filename) {

  if (dbHandle != NULL)
    closeFile();
  if (filename == "")
    return;

  // if the database file exists delete it
	std::ifstream ifile;
	ifile.open(filename.c_str());
	if(ifile) {
		//file exits so delete it
		ifile.close();
		if(remove(filename.c_str())!=0)
			NTA_THROW << "DatabaseRegion::openFile -- Error deleting existing database file! Filename:"
					  << filename;

	} else {
		ifile.close();
	}

  // create new file

  int result = sqlite3_open(filename.c_str(), &dbHandle);

  if (result!=0 || dbHandle==nullptr)
  {

    NTA_THROW
        << "DatabaseRegion::openFile -- unable to create database file: "
        << filename.c_str()
		<< " Error code:"
		<< result;
  }
  filename_ = filename;


  //number of disk pages that will be hold in memory, adjust this to set up size of the cache
  ExecuteSQLcommand("PRAGMA cache_size=10000");

}

static int SQLcallback(void *data, int argc, char **argv, char **azColName){
	//expecting to return rowcount only
	if(argc==1){
		std::stringstream strValue;//just convert string to unsigned int
		strValue << argv[0];
		strValue >> auxRowCnt;
	}else auxRowCnt = 0;

	return 0;
}

//iterates over all tables and sum up row count
UInt DatabaseRegion::getRowCount(){

	UInt sumRowCount = 0;

	for (UInt i = 0; i< iTableCount; i ++){
		std::string sTableName = "dataStream_dataIn"+std::to_string(i);

		std::string sql = "SELECT COUNT(*) FROM "+sTableName+";";

		char *zErrMsg;
		int returnCode = sqlite3_exec(dbHandle, sql.c_str(), SQLcallback, 0, &zErrMsg);


		if( returnCode != SQLITE_OK ){
			NTA_THROW << "Error counting rows in SQL table, message:"
						<< zErrMsg;
			sqlite3_free(zErrMsg);
			return 0;
		}else{
			sumRowCount+=auxRowCnt;
		}
	}
	return sumRowCount;
}

void DatabaseRegion::setParameterString(const std::string &paramName,
                                            Int64 index, const std::string &s) {

  if (paramName == "outputFile") {
    if (s == filename_)
      return; // already set
    if (dbHandle!=nullptr)
      closeFile();
    openFile(s);
  } else {
    NTA_THROW << "DatabaseRegion -- Unknown string parameter " << paramName;
  }
}

std::string DatabaseRegion::getParameterString(const std::string &paramName,
                                                   Int64 index) const {
  if (paramName == "outputFile") {
    return filename_;
  } else {
    NTA_THROW << "DatabaseRegion -- unknown parameter " << paramName;
  }
}

std::string
DatabaseRegion::executeCommand(const std::vector<std::string> &args,
                                   Int64 index) {
  NTA_CHECK(args.size() > 0);
  // Process the flushFile command
  if (args[0] == "closeFile") {
    closeFile();
  } else if (args[0] == "getRowCount") {
    return std::to_string(getRowCount());
  }else if (args[0] == "commitTransaction") {
  	if(xTransactionActive){
			ExecuteSQLcommand("END TRANSACTION");//ends transaction. Now it flushes cache to the file.
			xTransactionActive=false;
		}else NTA_THROW << "DatabaseRegion: Cannot commit transaction, transaction is not active!";
  }
  else {
    NTA_THROW << "DatabaseRegion: Unknown execute '" << args[0] << "'";
  }

  return "";
}

Spec *DatabaseRegion::createSpec() {

  auto ns = new Spec;
  ns->name = "DatabaseRegion";
  ns->description =
      "DatabaseRegion is a node that writes multiple scalar streams "
      "to a SQLite3 database file (.db). The target filename is specified "
      "using the 'outputFile' parameter at run time. On each "
      "compute, all inputs are written "
      "to the database.";

  for (UInt i = 0; i< MAX_NUMBER_OF_INPUTS; i ++){ // create 10 inputs, user don't have to use them all
		ns->inputs.add("dataIn"+std::to_string(i),
								InputSpec("Data scalar to be written to the database",
													NTA_BasicType_Real32,
													0,     // count
													false, // required?
													true, // isRegionLevel
													true   // isDefaultInput
													));
  }

  ns->parameters.add("outputFile",
              ParameterSpec("Writes data stream to this database file on each "
                            "compute. Database is recreated on initialization "
                            "This parameter must be set at runtime before "
                            "the first compute is called. Throws an "
                            "exception if it is not set or "
                            "the file cannot be written to.",
                            NTA_BasicType_Str,
                            1,  // elementCount
                            "", // constraints
                            "", // defaultValue
                            ParameterSpec::ReadWriteAccess));

  ns->commands.add("closeFile",
                   CommandSpec("Close the current database file, if open."));
  ns->commands.add("getRowCount",
                     CommandSpec("Gets sum of row counts for all tables in opened database."));
  ns->commands.add("commitTransaction",
      CommandSpec("Commits currently active transaction. Speeding up write avoiding repeat writes in loop."
      						"Transaction is started when databse is opened."));


  return ns;
}

size_t DatabaseRegion::getNodeOutputElementCount(const std::string &outputName) const {
  NTA_THROW
      << "DatabaseRegion::getNodeOutputElementCount -- unknown output '"
      << outputName << "'";
}


bool DatabaseRegion::operator==(const RegionImpl &o) const {
  if (o.getType() != "DatabaseRegion") return false;
  DatabaseRegion& other = (DatabaseRegion&)o;
  if (filename_ != other.filename_) return false;

  return true;
}

} // namespace htm
