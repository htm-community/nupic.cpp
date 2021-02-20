/* ---------------------------------------------------------------------
 * HTM Community Edition of NuPIC
 * Copyright (C) 2020, Numenta, Inc.
 *
 * Author:  David Keeney, 07/02/2020  dkeeney@gmail.com
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


#include <htm/utils/Log.hpp>
#include "sqlite3.h"
#include "gtest/gtest.h"

namespace testing { 
    
using namespace htm;

static int callback(void *NotUsed, int argc, char **argv, char **azColName){
  // Normally you would do something here with the results.
  return 0;
}

TEST(SQLiteTest, hello)
{
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;
  const char *dbName = ":memory:";  // A private temporary database is created in memory and is deleted on close.
  

  rc = sqlite3_open(dbName, &db);
  if ( rc ) {
    std::string err = sqlite3_errmsg(db);
    sqlite3_close(db);
    FAIL() << "sqlite3 Can't open database: " << err;
  }
  
  std::string sql = "create table tbl1(one varchar(10), two smallint);";
  rc = sqlite3_exec(db, sql.c_str(), callback, 0, &zErrMsg);
  if( rc!=SQLITE_OK ){
    std::string err = zErrMsg;
    sqlite3_free(zErrMsg);
    FAIL() << "SQL error: " << err;
  }
  sqlite3_close(db);
}

} // testing namespace
