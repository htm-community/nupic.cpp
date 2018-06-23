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
*/

#include <string>
#include <algorithm>
#include <chrono>   // for sleep_for()
#include <thread>
#include <boost/filesystem.hpp>

#include <nupic/utils/Log.hpp>
#include <nupic/os/Directory.hpp>
#include <nupic/os/Path.hpp>
#include <nupic/os/OS.hpp>
#include <nupic/utils/Log.hpp>
using namespace boost::system;
namespace fs = boost::filesystem;
using namespace std::literals::chrono_literals;


namespace nupic
{
  namespace Directory
  {
    bool exists(const std::string & fpath)
    {
      return fs::exists(fpath);
    }
    
    std::string getCWD()
    {
        return fs::current_path().string();
    }

    bool empty(const std::string & path)
    {
        return fs::is_empty(path);
    }

    Size free_space(const std::string & path) {
      fs::space_info si = fs::space(path);
      return (Size)si.available;  // disk space available to non-privalaged pocesses.
    }
    
    void setCWD(const std::string & path)
    {
       fs::current_path(path);
        //std::this_thread::sleep_for(100ms);  // give it time to propogate
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    static bool removeEmptyDir(const std::string & path, bool noThrow)
    {
        error_code ec;
        fs::remove(path, ec);
        if(!noThrow) {
            NTA_CHECK(!ec) << "removeEmptyDir: " << ec.message();
        }
        return(!ec);
    }

    // copy a directory recursively.
    // It does not copy links.  It tries to retain permissions.
    // If a file already exists in the destination it is overwritten.
    void copyTree(const std::string & source, const std::string & destination)
    {
      error_code ec;

      NTA_CHECK(Path::isDirectory(source)) << "copyTree() source is not a directory. " << source;
      if (Path::exists(destination)) {
        NTA_CHECK(Path::isDirectory(destination)) << "copyTree() destination exists '" << destination << "' and it is not a directory.";
      }
      else {
        fs::copy_directory(source, destination, ec); // creates directory, copying permissions
        NTA_CHECK(!ec) << "copyTree: Could not create destination directory. '" << destination << "' " << ec.message();
      }
      Directory::Iterator it(source);
      Directory::Entry entry;
      while (it.next(entry) != nullptr) {
        // Note: this does not copy links.
        std::string to = destination + Path::sep + entry.filename;
        if (entry.type == Entry::FILE) {
          fs::copy_file(entry.path, to, fs::copy_option::overwrite_if_exists, ec);
        }
        else if (entry.type == Entry::DIRECTORY) {
          copyTree(entry.path, to);
        }
      }

    }

    bool removeTree(const std::string & path, bool noThrow)
    {
      error_code ec;
      if (fs::is_directory(path, ec)) {
        fs::remove_all(path, ec);
      }
      if (!noThrow) {
        NTA_CHECK(!ec) << "removeTree: " << ec.message();
      }
      return(!ec);
    }

    // create directory
    void create(const std::string& path, bool otherAccess, bool recursive)
    {
      NTA_CHECK(!path.empty()) << "Directory::create -- Can't create directory with no name";
      fs::path p = fs::absolute(path);
      if (fs::exists(p)) {
        if (!fs::is_directory(p)) {
          NTA_THROW << "Directory::create -- path " << path << " already exists but is not a directory";
        }
        return;
      }
      else {
        if (recursive) {
          error_code ec;
          if (!fs::create_directories(p, ec)) {
            NTA_CHECK(!ec) << "Directory::createRecursive: " << ec.message();
          }
        } 
        else {
          // non-recursive case
          NTA_CHECK(fs::exists(p.parent_path())) << "Directory::create -- path " << path << " Parent directory does not exist.";
          error_code ec;
          fs::create_directory(p, ec);
          NTA_CHECK(!ec) << "Directory::create " << ec.message();
        }
      }
      // Set permissions on directory.
#if !defined(NTA_OS_WINDOWS)
      fs::perms prms(fs::perms::owner_all
        | (otherAccess ? (fs::perms::group_all | fs::perms::others_read | fs::perms::others_exe) : fs::perms::no_perms));
      fs::permissions(p, prms);
#endif
    }
    



    
  }
}
