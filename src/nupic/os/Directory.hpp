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

/** @file */


#ifndef NTA_DIRECTORY_HPP
#define NTA_DIRECTORY_HPP

//----------------------------------------------------------------------

#include <string>
#include <filesystem>
#include <nupic/os/Path.hpp>


//----------------------------------------------------------------------

namespace nupic
{
  
  namespace Directory
  {
    // check if a directory exists
    bool exists(const std::string & path);

    // true if the directory is empty
    bool empty(const std::string & path);  

    // return the amount of available space on this path's device.
    Size free_space(const std::string & path);
    
    // get/set current working directory
    std::string getCWD();
    void setCWD(const std::string & path); //  be careful about using this.

    // Copy directory tree rooted in 'source' to 'destination'
    void copyTree(const std::string & source, const std::string & destination);
    		
    // Remove directory tree rooted in 'path'
    bool removeTree(const std::string & path, bool noThrow=false);
		
    // Create directory 'path' including all parent directories if missing (and if recursive is true)
    // sets permissions read, write, execute for owner (0700). 
    // if otherAccess = true, sets read,write,execute for group and read,execute for others (0775).
    // Failures will throw an exception
    void create(const std::string & path, bool otherAccess=false, bool recursive=false);

    //std::string createTemporary(const std::string &templatePath);  Not implemented

    struct Entry
    {
      enum Type { FILE, DIRECTORY, LINK, OTHER };
			
      Type type;
      std::string path;         // full absolute path
      std::string filename;     // just the filename and extension or directory name
    };

    class Iterator
    {
    public:
      Iterator(const nupic::Path & path) {
        std::string pstr = path.c_str();
        p_ = std::filesystem::absolute(pstr);
        current_ = std::filesystem::directory_iterator(p_);
      }
      Iterator(const std::string & path) {
        p_ = std::filesystem::absolute(path);
        current_ = std::filesystem::directory_iterator(p_);
      }
      ~Iterator() {}
		
      // Resets directory to start. Subsequent call to next() 
      // will retrieve the first entry
      void reset() { 
        current_ = std::filesystem::directory_iterator(p_);
      }

      // get next directory entry
      Entry * next(Entry & e) {
        std::error_code ec;
        if (current_ == end_)  return nullptr;
        e.type = (std::filesystem::is_directory(current_->path(), ec)) ? Entry::DIRECTORY : 
                 (std::filesystem::is_regular_file(current_->path(), ec)) ? Entry::FILE :
                 (std::filesystem::is_symlink(current_->path(), ec))? Entry::LINK : Entry::OTHER;
        e.path = current_->path().string();
        e.filename = current_->path().filename().string();
        current_++;
        return &e;
      }
		
    private:
      Iterator() {}    
      Iterator(const Iterator &) {}
		
    private:
      std::filesystem::path p_;
      std::filesystem::directory_iterator current_;
      std::filesystem::directory_iterator end_;

    };
  }
}

#endif // NTA_DIRECTORY_HPP


