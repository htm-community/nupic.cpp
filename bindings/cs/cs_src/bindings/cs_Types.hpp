/* ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (Nupic)
# Copyright (C) 2019, Numenta, Inc.  Unless you have an agreement
# with Numenta, Inc., for a separate license for this software code, the
# following terms and conditions apply:
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the GNU Affero Public License for more details.
#
# You should have received a copy of the GNU Affero Public License
# along with this program.  If not, see http://www.gnu.org/licenses.
#
# http://numenta.org/licenses/
#
# Author: David Keeney, Jan 2018
# ----------------------------------------------------------------------*/

  /** @file
  * Utility conversion functions 
  * This file contains some type conversion routines.
  * These are routines that are intended to be used for coding the C++ to C#
  * interface and are not expected to be very useful within C# except
  * cs_Log and cs_LoggingException.
  *
  * Namespace htm       -- for internal classes that should not be accessed by C#
  * Namespace htm::core -- for wrappered classes that are exposed to C#
  * 
  * This is part of the Cpp/CLI code that forms the API interface for C#.
  * compile with: /clr
  * compile with: /EHa 
  */


#ifndef NTA_CS_TYPES_HPP
#define NTA_CS_TYPES_HPP
#pragma once

#include <eh.h>      // for exception handling
#include <vcclr.h>  // for gcroot() and PtrToStringChars()
#include <vector>  
#include <typeinfo>
#pragma make_public(std::type_info)  
#include <fstream>
#include <string>

#define generic generic_t 
#include <htm/engine/htm.hpp>
#include <htm/engine/Link.hpp>
#include <htm/engine/Network.hpp>
#include <htm/engine/Region.hpp>
#include <htm/engine/Spec.hpp>
#include <htm/engine/RegisteredRegionImpl.hpp>
#include <htm/ntypes/Dimensions.hpp>
#include <htm/ntypes/Collection.hpp>
#include <htm/ntypes/BundleIO.hpp>
#include <htm/os/Timer.hpp>
#include <htm/utils/LogItem.hpp>   // for htm.core Logging facility
#include <htm/types/Types.hpp>     // for NTA_BasicType
#include <htm/types/Exception.hpp> // for htm.core base Exception class
#include <htm/ntypes/Array.hpp>     // for NTA_BasicType
#include <htm/utils/LogItem.hpp>   // for htm.core Logging facility
#undef generic
#pragma make_public(htm::Exception)  
#pragma make_public(htm::LogItem)  


#include <cs_Tools.h>  

using namespace System;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;    // for GCHandle
using namespace htm;
namespace htm_core_cs
{ 
  class ArrayCS;
  ref class cs_Log;
  ref class cs_LoggingException;




  // This causes the Managed DLL to be loaded and initialized.
  // In the main() call EnsureManagedInitialization();
  // See https://docs.microsoft.com/en-us/cpp/dotnet/how-to-migrate-to-clr
  __declspec(dllexport) void EnsureManagedInitialization() {
    // managed code that won't be optimized away  
    System::GC::KeepAlive(System::Int32::MaxValue);
  }


  //////////////////////////////////////////////////
  ///        Conversion Routines                 ///
  //////////////////////////////////////////////////
      // dynamiclly load the dll
      //static void InitializeLibrary(System::String^ path);

      // NOTE: #include <msclr\marshal_cppstd.h> has nice conversion routines but
      //       they do NOT have UTF8 conversion.

      // std::string and System::String^  (does copy)
      static System::String^ UTF8ToString(const std::string s) {
        size_t len = s.length();
        wchar_t* wcstring = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
        size_t convertedChars = 0;
        errno_t err = 0;
        err = mbstowcs_s(&convertedChars, wcstring, len + 1, s.c_str(), _TRUNCATE);
        String^ st;
        if (err != 0)
          st = gcnew System::String("");
        else
          st = gcnew System::String(wcstring, 0, (int)convertedChars);
        free(wcstring);
        return st;
      }
      static std::string StringToUTF8(System::String^ s) {
        // The managed System::String stores strings as wide characters (UTF16).
        // Pin memory so GC can't move it while native function is called  
        // Then convert that to a char * using wcstombs_s() to create UTF8 string.
        pin_ptr<const wchar_t> wch = PtrToStringChars(s);
        size_t convertedChars = 0;
        size_t  sizeInBytes = (size_t)((s->Length + 1) * 4);
        char    *ch = (char *)malloc(sizeInBytes);

        errno_t err = 0;
        err = wcstombs_s(&convertedChars, ch, sizeInBytes, wch, s->Length);
        if (err != 0)
          return std::string();
        std::string st = std::string(ch, convertedChars);
        free(ch);
        return st;
      }

      // std::vector and List  (does copy)  Only good for UInt64
      static System::Collections::Generic::List<System::UInt64>^ vectorToList(std::vector<htm::UInt64>& vec) {
        System::Collections::Generic::List<System::UInt64>^ lst = gcnew System::Collections::Generic::List<System::UInt64>(vec.size());
        for (auto i : vec) {
          lst->Add(i);
        }
        return lst;
      }
      static std::vector<size_t> ListTovector(System::Collections::Generic::List<System::UInt64>^ lst) {
        std::vector<size_t> v;
        for (auto i = 0; i < lst->Count; i++) {
          v.push_back(lst[i]);
        }
        return v;
      }

      // retreive NTA_BasicType from template T
      static NTA_BasicType parseT(const std::type_info& t) {
        NTA_BasicType type;
        if (t == typeid(System::Byte))
          type = NTA_BasicType_Byte;
        else if (t == typeid(System::Int16))
          type = NTA_BasicType_Int16;
        else if (t == typeid(System::UInt16))
          type = NTA_BasicType_UInt16;
        else if (t == typeid(System::Int32))
          type = NTA_BasicType_Int32;
        else if (t == typeid(System::UInt32))
          type = NTA_BasicType_UInt32;
        else if (t == typeid(System::Int64))
          type = NTA_BasicType_Int64;
        else if (t == typeid(System::UInt64))
          type = NTA_BasicType_UInt64;
        else if (t == typeid(System::Single))
          type = NTA_BasicType_Real32;
        else if (t == typeid(System::Double))
          type = NTA_BasicType_Real64;
        else if (t == typeid(System::Boolean))
          type = NTA_BasicType_Bool;
        else {
          //System::String^ msg = L"Invalid type name: " + UTF8ToString(t.name());
          //throw gcnew System::Exception(msg);
          type = NTA_BasicType_Last;  // This means there was an error -- type not found.
        }
        return type;
      }

      // retreive NTA_BasicType from generic T
      static NTA_BasicType parseT(System::Type^ t) {
        NTA_BasicType type;
        if (t == System::Byte::typeid)
          type = NTA_BasicType_Byte;
        else if (t == System::Int16::typeid)
          type = NTA_BasicType_Int16;
        else if (t == System::UInt16::typeid)
          type = NTA_BasicType_UInt16;
        else if (t == System::Int32::typeid)
          type = NTA_BasicType_Int32;
        else if (t == System::UInt32::typeid)
          type = NTA_BasicType_UInt32;
        else if (t == System::Int64::typeid)
          type = NTA_BasicType_Int64;
        else if (t == System::UInt64::typeid)
          type = NTA_BasicType_UInt64;
        else if (t == System::Single::typeid)
          type = NTA_BasicType_Real32;
        else if (t == System::Double::typeid)
          type = NTA_BasicType_Real64;
        else if (t == System::Boolean::typeid)
          type = NTA_BasicType_Bool;
        else {
          //System::String^ msg = L"Invalid type name: " + UTF8ToString(t.name());
          //throw gcnew System::Exception(msg);
          type = NTA_BasicType_Last;  // This means there was an error -- type not found.
        }
        return type;
      }


      generic <typename T>
        static T getManagedValue(void *ptr, int i) {
          if (T::typeid == System::Byte::typeid)
            return (T)(((unsigned char*)ptr)[i]);
          else if (T::typeid == System::Int16::typeid)
            return (T)(((short*)ptr)[i]);
          else if (T::typeid == System::UInt16::typeid)
            return (T)(((unsigned short*)ptr)[i]);
          else if (T::typeid == System::Int32::typeid)
            return (T)(((htm::Int32*)ptr)[i]);
          else if (T::typeid == System::UInt32::typeid)
            return (T)(((htm::UInt32*)ptr)[i]);
          else if (T::typeid == System::Int64::typeid)
            return (T)(((htm::Int64*)ptr)[i]);
          else if (T::typeid == System::UInt64::typeid)
            return (T)(((htm::UInt64*)ptr)[i]);
          else if (T::typeid == System::Single::typeid)
            return (T)(((float*)ptr)[i]);
          else if (T::typeid == System::Double::typeid)
            return (T)(((double*)ptr)[i]);
          else if (T::typeid == System::Boolean::typeid)
            return (T)(((bool*)ptr)[i]);
          else {
            throw gcnew System::Exception("Invalid type in getManagedValue()");
          }
          return (T)0;
        }

        generic <typename T>
          static void setManagedValue(T val, void *ptr, int i) {
            if (T::typeid == System::Byte::typeid)
              ((unsigned char*)ptr)[i] = (unsigned char)val;
            else if (T::typeid == System::Int16::typeid)
              ((short*)ptr)[i] = (short)val;
            else if (T::typeid == System::UInt16::typeid)
              ((unsigned short*)ptr)[i] = (unsigned short)val;
            else if (T::typeid == System::Int32::typeid)
              ((htm::Int32*)ptr)[i] = (htm::Int32)val;
            else if (T::typeid == System::UInt32::typeid)
              ((htm::UInt32*)ptr)[i] = (htm::UInt32)val;
            else if (T::typeid == System::Int64::typeid)
              ((htm::Int64*)ptr)[i] = (htm::Int64)val;
            else if (T::typeid == System::UInt64::typeid)
              ((htm::UInt64*)ptr)[i] = (htm::UInt64)val;
            else if (T::typeid == System::Single::typeid)
              ((float*)ptr)[i] = (float)val;
            else if (T::typeid == System::Double::typeid)
              ((double*)ptr)[i] = (double)val;
            else if (T::typeid == System::Boolean::typeid)
              ((bool*)ptr)[i] = (bool)val;
            else {
              throw gcnew System::Exception("Invalid type in setManagedValue()");
            }
          }

    



      /* This class represents a managed array allocated in C# and passed into the engine(non - managed code)
      * Inside the unmanaged code we need to have it be treated like an unmanaged array so it must be pinned.
      * The pin must not go out of scope while it is being used so we put it on this subclass of the Array class
      * wrapped in a GCHandle.  Then we can pass this subclass to the engine as if it were the Array class.
      * NOTE: this is non-managed code used internally by the wrapper code. Not to be accessed by C#
      */
    public class ArrayCS : public htm::Array
    {
    public:
      //   t is typeid(T), arr is an array<T>, count is size of array
      ArrayCS(const std::type_info& t, System::Array^ arr, System::UInt64 count) : htm::Array(parseT(t)) {
        original_ = arr;       // set it aside for eventual retreival
        // lock down the first element which locks down everything.
        p_ = GCHandle::Alloc(((array<System::Byte>^)arr)[0], GCHandleType::Pinned);
        buffer_ =(char*) p_.AddrOfPinnedObject().ToPointer();
        count_ = count;
        own_ = false;  // don't want ArrayBase to free it
        localOwn_ = false;
        allocator = nullptr;
      }
      //   t is typeid(T), alloc is an allocator function that returns an array of type T, argument is number of elements.
      ArrayCS(const std::type_info& t, System::Array^ (*alloc)(System::UInt64)) :  htm::Array(parseT(t)) {
        allocator = alloc;
        original_ = nullptr;       // set it aside for eventual retreival
        localOwn_ = false;
      }

      virtual ~ArrayCS() { 
        p_.Free();
        if (localOwn_) delete original_;
      }

      // This is so the engine could allocate or resize the array as needed.
      virtual void allocateBuffer(size_t count) override {
        if (allocator) {
          if (localOwn_) delete original_;
          original_ = allocator(count);
          System::Array^ arr = original_;
          // lock down the first element which locks down everything.
          interior_ptr<unsigned char> ptr = &((array<System::Byte>^)arr)[0];  // get the first element of the array
          p_ = GCHandle::Alloc(*ptr, GCHandleType::Pinned);
          buffer_ = (char*) p_.AddrOfPinnedObject().ToPointer();
          count_ = count;
          own_ = false;  // do not want ArrayBase to free it
          localOwn_ = true;
        }
        else {
          // TODO: allocate based on NTA_BasicType
          throw htm::Exception(__FILE__, __LINE__, "ArrayCS::allocateBuffer(), Allocator not provided.");
        }
      }

      Object^ getManaged() { return original_; }

    private:
      gcroot<System::Array^> original_;
      GCHandle p_;    // pinned buffer elements so garbage collector cannot move them.
      bool localOwn_;      // note: own_ in ArrayBase is different
      System::Array^ (*allocator)(System::UInt64);  // a function that can allocate an array of the approprate type.

    };

    


    //////////////////////////////////////////////////
    ///             Log                            ///
    //////////////////////////////////////////////////
    //   Used in C# as:
    //     cs_Log::setOutputFile(fullpathname);
    //     CS_Log::close();
    //
    //     CS_LDEBUG(string msg, [CallerFilePath] string path = null, [CallerMemberName] string module = null, [CallerLineNumber] int line=-1) {
    //         cs_Log::write(msg, path, module, line, LogLevel.debug);
    //     }
    //     CS_DEBUG(string msg) {
    //         cs_Log::write(msg, LogLevel.debug);
    //     }
    //   Simuler for CS_INFO, CS_WARN, CS_CHECK CS_ASSERT    (derived from htm/util/Log.hpp)
    //
    public ref class cs_Log {
    public:
      static void setOutputFile(String^ filename) {
        std::ofstream* fout = new std::ofstream();
        fout->open(StringToUTF8(filename), std::ios::out);
        htm::LogItem::setOutputFile(*fout);
      }

      static void write(System::String^ msg, htm::LogItem::LogLevel level) {
        htm::LogItem *li = new htm::LogItem("", 0, level);
        li->stream() << StringToUTF8(msg);
        delete li;
      }
      static void write(System::String^ msg, System::String^ module, System::String^ path, System::Int32 line, htm::LogItem::LogLevel level) {
        // use the LogItem class for things logged in C# so that the unmanaged code uses the same log file.
        System::String^ s = path->Substring(path->LastIndexOfAny(gcnew array<Char>(2) { L'/', L'\\' }) + 1);
        if (module->Length > 0) s += ":" + module;
        std::string f = StringToUTF8(s);
        htm::LogItem *li = new htm::LogItem(f.c_str(), line, level);
        li->stream() << StringToUTF8(msg) << std::endl;
        delete li;
      }
    };

    //////////////////////////////////////////////////
    ///          LoggingException                  ///
    //////////////////////////////////////////////////

    public ref class cs_LoggingException : public System::Exception
    {
    public:
      cs_LoggingException(htm::Exception* ex) { message_ = UTF8ToString(ex->getMessage()); }
      cs_LoggingException(const char* msg) { message_ = L"Exception: " + UTF8ToString(msg); }
      cs_LoggingException(System::String^ msg) { message_ = L"Exception: " + msg; }
      cs_LoggingException(System::String^ msg, System::String^ membername, System::String^ path, System::UInt32 lineno) {
        System::String^ filename = path->Substring(path->LastIndexOfAny(gcnew array<Char>(2) { L'/', L'\\' }) + 1);
        message_ = L"Exception::[" + membername + L" " + filename + L"(" + lineno + L")]" + msg;
      }
      cs_LoggingException(const char* path, System::UInt32 lineno, const char* msg) {   // for use with CS_THROW and CHKEXP macros
        const char* p1 = strrchr(path, '/');
        const char* p2 = strrchr(path, '\\');
        p1 = (p1 > p2) ? (p1 + 1) : (p2) ? (p2 + 1) : path;
        char buffer[200];
        sprintf_s(buffer, sizeof(buffer), "[%s (%d)]", p1, (int)lineno);
        message_ = UTF8ToString(std::string(buffer) + std::string(msg));
      }


      virtual System::String^ getMessage() { return message_; }
      virtual void logMessage() { cs_Log::write(getMessage(), htm::LogItem::LogLevel::error); }

    private:
      String ^ message_;       // contains context
    }; // class LoggingException    




} // namespace htm_core_cs

#endif  // NTA_CS_TYPES_HPP