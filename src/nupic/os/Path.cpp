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

#include <nupic/os/Path.hpp>
#include <nupic/os/Directory.hpp>
#include <nupic/os/OS.hpp>
#include <nupic/os/FStream.hpp>
#include <nupic/utils/Log.hpp>
#include <boost/tokenizer.hpp>
#include <boost/scoped_array.hpp>

#include <codecvt>

#include <sstream>
#include <string>
#include <iterator>
#include  <stdio.h>  
#include  <stdlib.h>  
#if defined(NTA_OS_WINDOWS)
#include  <io.h>  
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

// using boost::filesystem
#include <boost/filesystem.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/algorithm/string/replace.hpp>
using namespace boost::system;
namespace fs = boost::filesystem;



namespace nupic
{
#if defined(NTA_OS_WINDOWS)
    const char * Path::sep = "\\";
    const char * Path::pathSep = ";";
#else
    const char * Path::sep = "/";
    const char * Path::pathSep = ":";
#endif
    const char * Path::parDir = "..";


    bool Path::exists(const std::string & path)
    {
      return fs::exists(path);
    }

    bool Path::equals(const std::string& path1, const std::string& path2)
    {
      std::string s1 = normalize(path1);
      boost::replace_all(s1, "\\", "/");
      std::string s2 = normalize(path2);
      boost::replace_all(s2, "\\", "/");
      return (s1 == s2);
    }


    bool Path::isFile(const std::string & path)
    {
      return fs::is_regular_file(path);
    }

    bool Path::isDirectory(const std::string & path)
    {
      return fs::is_directory(path);
    }

    bool Path::isSymbolicLink(const std::string & path)
    {
      return fs::is_symlink(path);
    }

    bool Path::isAbsolute(const std::string & path)
    {
#if defined(NTA_OS_WINDOWS)
      if (path.length() == 2 && isalpha(path[0]) && path[1] == ':') return true;  //  c:
      if (path.length() >= 3 && path[0] == '\\' && path[1] == '\\' && isalpha(path[3])) return true;  // \\net
#endif
      return fs::path(path).is_absolute();
    }

    std::string Path::normalize(const std::string & path)
    {
      // Note: I expected to use system_complete() for this but it requires the path to exist.
      if (path.empty()) return path;
      fs::path p(path);
      p = p.lexically_normal();
      return p.string();
    }


    bool Path::areEquivalent(const std::string & path1, const std::string & path2)
    {
      fs::path p1(path1);
      fs::path p2(path2);
      if (!fs::exists(p1) || !fs::exists(p2))
        return false;
      // must resolve to the same existing filesystem and file entry to match.
      return fs::equivalent(p1, p2);
    }

    std::string Path::getParent(const std::string & path)
    {
        if (path.empty()) return path;
        if (path == ".") return "..";
        fs::path p(path);
        if (p.root_path() == path) return path; // parent of root is the root
        p = p.lexically_normal(); // remove .. and . if possible
        if (p.filename_is_dot_dot()) return p.string() + "/..";
        p = p.parent_path();
        if (p.string().empty())
          return ".";
        return p.string();
    }

    std::string Path::getBasename(const std::string & path)
    {
      if (path.empty()) return path;
      fs::path p(path);
      if (p.has_filename()) {
        std::string name = p.filename().string();
        return name;
      }
      return "";
    }

    std::string Path::getExtension(const std::string & path)
    {
      if (path.empty()) return path;
      fs::path p(path);
      if (p.has_extension())
        return p.extension().string().substr(1);  // do not include the .
      return "";
    }


    Size Path::getFileSize(const std::string & path)
    {
      return (Size)fs::file_size(path);
    }


    std::string Path::makeAbsolute(const std::string & path)
    {
      return fs::absolute(path).string();
    }

    // determine if it is something like  C:\ on windows or / or linux. or perhaps //hostname
    bool Path::isRootdir(const std::string& s)
    {
      fs::path p(s);
      return (p.root_path().string() == s);
    }



    std::string Path::unicodeToUtf8(const std::wstring& wstr)
    {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
      return myconv.to_bytes(wstr);
    }

    std::wstring Path::utf8ToUnicode(const std::string& str)
    {
      std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
      return myconv.from_bytes(str);
    }

    void Path::copy(const std::string & source, const std::string & destination)
    {
        NTA_CHECK(!source.empty())
            << "Can't copy from an empty source";

        NTA_CHECK(!destination.empty())
            << "Can't copy to an empty destination";

        NTA_CHECK(!areEquivalent(source, destination))
            << "Source and destination must be different";

        if (isDirectory(source))
        {
            Directory::copyTree(source, destination);
            return;
        }
        fs::path parent = fs::path(destination).parent_path();
        error_code ec;
        if (!parent.string().empty() && !fs::exists(parent)) {
          fs::create_directories(parent, ec);
          NTA_CHECK(!ec) << "Path::copy - failure creating destination path '" << destination << "'" << ec.message();
        }
        fs::copy_file(source, destination, ec);
        NTA_CHECK(!ec) << "Path::copy - failure copying file '" << source <<"' to '" << destination << "'" << ec.message();

    }

    void Path::setPermissions(const std::string &path,
        bool userRead, bool userWrite,
        bool groupRead, bool groupWrite,
        bool otherRead, bool otherWrite
    )
    {
        if (Path::isDirectory(path)) {
          fs::perms prms =
            (userRead ? fs::owner_exe | fs::owner_read : fs::no_perms)
            | (userWrite ? fs::owner_all : fs::no_perms)
            | (groupRead ? fs::group_exe | fs::group_read : fs::no_perms)
            | (groupWrite ? fs::group_all : fs::no_perms)
            | (otherRead ? fs::others_exe | fs::others_read : fs::no_perms)
            | (otherWrite ? fs::others_all : fs::no_perms)
            ;
          fs::permissions(path, prms);

            Directory::Iterator iter(path);
            Directory::Entry e;
            while (iter.next(e)) {
                setPermissions(e.path,
                    userRead, userWrite,
                    groupRead, groupWrite,
                    otherRead, otherWrite);
            }
        }
        else {
          fs::perms prms = 
              (userRead ? fs::owner_read : fs::no_perms)
            | (userWrite ? fs::owner_write : fs::no_perms)
            | (groupRead ? fs::group_read : fs::no_perms)
            | (groupWrite ? fs::group_write : fs::no_perms)
            | (otherRead ? fs::others_read : fs::no_perms)
            | (otherWrite ? fs::others_write : fs::no_perms)
            ;
            fs::permissions(path, prms);
        }
    }

    void Path::remove(const std::string & path)
    {
        NTA_CHECK(!path.empty())  << "Can't remove an empty path";

        // Just return if it doesn't exist already
        if (!Path::exists(path))
            return;

        if (isDirectory(path))
        {
            Directory::removeTree(path);
            return;
        }
        error_code ec;
        fs::remove(path, ec);
        NTA_CHECK(!ec) << "Path::remove - failure removing file '" << path << "'" << ec.message();
    }


    void Path::rename(const std::string & oldPath, const std::string & newPath)
    {
        NTA_CHECK(!oldPath.empty() && !newPath.empty())   << "Can't rename to/from empty path";
        fs::path oldp = fs::absolute(oldPath);
        fs::path newp = fs::absolute(newPath);

        error_code ec;
        fs::rename(oldp, newp, ec);
        NTA_CHECK(!ec) << "Path::remove - failure renaming file '" << oldp.string() << "' to '" << newp.string() << "'" << ec.message();
    }


    void Path::write_all(const std::string& filename, const std::string& value)
    {
      OFStream f(filename.c_str());
      f << value;
      f.close();
    }

    std::string Path::read_all(const std::string& filename)
    {
      std::string s;
      IFStream f(filename.c_str());
      f >> s;
      return s;
    }


    /*******************************************
    * This function returns the full path of the executable that is running this code.
    * see https://stackoverflow.com/questions/1023306/finding-current-executables-path-without-proc-self-exe
    *     https://stackoverflow.com/questions/1528298/get-path-of-executable
    * The above links show that this is difficult to do and not 100% reliable in the general case.
    * However, the boost solution is one of the best.
    */
    std::string Path::getExecutablePath()
    {
      error_code ec;
      fs::path p = boost::dll::program_location(ec);
      NTA_CHECK(!ec) << "Path::getExecutablePath() Fail. " << ec.message();
      return p.string();
    }

    // Global operators
    Path operator+(const Path & p1, const Path & p2) { return Path(std::string(*p1) + Path::sep + std::string(*p2)); }
    Path operator+(const std::string & p1, const Path & p2) { return Path(p1 + Path::sep + std::string(*p2)); }
    Path operator+(const Path & p1, const std::string & p2) { return Path(std::string(*p1) + Path::sep + p2); }

} // namespace nupic
