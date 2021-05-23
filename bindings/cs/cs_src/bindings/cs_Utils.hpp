/* ----------------------------------------------------------------------
# Numenta Platform for Intelligent Computing (NuPIC)
# Copyright (C) 2018, Numenta, Inc.  Unless you have an agreement
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
  * Wrappers for Utility functions 
  * These are routines that are C++/CLI wrappers for core classes so thy
  * can be accessed within C#.
  * 
  * This is part of the Cpp/CLI code that forms the htm::Core API interface for C#.
  * compile with: /clr
  * compile with: /EHa 
  */


#ifndef NTA_CS_UTILS_HPP
#define NTA_CS_UTILS_HPP
#pragma once

#include <vector>  
#include <typeinfo>
#include <fstream>
#include <string>

#include <cs_Tools.h>  // macors and static functions

#define generic generic_t 
#include <htm/ntypes/Array.hpp>
#include <htm/ntypes/ArrayRef.hpp>
#include <htm/ntypes/Value.hpp>
#include <htm/os/Timer.hpp>
#undef generic

using namespace System;
using namespace System::Runtime::InteropServices;    // for GCHandle
using namespace htm;

namespace htm_core_cs
{
  generic <typename T> ref class cs_Array;
  generic <typename T> ref class cs_ArrayRef;
  ref class cs_BundleIO;
  ref class cs_Timer;
  ref class cs_Scalar;
  ref class cs_Value;
  ref class cs_ValueMap;


  // Note: boost::shared_ptr is a problem in c++/cli because
  //       the unmanaged values cannot be members of a managed class
  //       although we can have a pointer to an unmanaged class as a
  //       member of a managed class. In order to allow the smart pointer to work properly
  //       we must have an instance of the smart pointer not a reference to it.
  //       So we allocate a class (indirect_smart_ptr) to hold the smart pointer instance
  //       and keep a pointer to the structure on our managed class.  Messy but it should work.
  //       When that class is deleted it must delete the struct containing the shared_ptr.
  template<typename T> 
  public class indirect_smart_ptr 
  { 
    public: 
      std::shared_ptr<T> p; 
  };


  /**********************************************
  *  A note about templates and generic types:
  *
  *  It is possible to instantiate a generic type with a managed type template parameter, 
  *  but you cannot instantiate a managed template with a generic type template parameter. 
  *  This is because generic types are resolved at runtime.
  *  https://msdn.microsoft.com/en-us/library/ms177213.aspx
  *
  *  Templates can be used on/in both managed and unmanaged classes.
  *  Generic types can only be used on managed classes.
  *
  *  Templates are not visible in C# so you cannot have a class or function that uses
  *  templates that you want to be usable or callable from C#.
  *  More about templates: 
  *  https://stackoverflow.com/questions/740969/c-sharp-generics-vs-c-templates-need-a-clarification-about-constraints
  **********************************************/


    //////////////////////////////////////////////////
    ///          Array                             ///
    //////////////////////////////////////////////////
    // This represents a buffer owned by C# (or the interface)
    //
    generic <typename T>
    public ref class cs_Array
    {
    internal:
      cs_Array(htm::Array* a) { 
        array_ = new indirect_smart_ptr<htm::Array>(); 
        array_->p.reset(a);
        arr_ = nullptr; 
      }
      cs_Array(boost::shared_ptr<htm::Array>& a) { 
        array_ = new indirect_smart_ptr<htm::Array>(); 
        array_->p = a; 
        arr_ = nullptr; 
      }
      array<T>^ getOriginal() { return arr_; }

    public:
      cs_Array(array<T>^ arr)
      {
        NTA_BasicType type = parseT(T::typeid);
        int count = arr->GetLength(0);
        arr_ = arr;
        pin_ = GCHandle::Alloc(arr, GCHandleType::Pinned);
        void *buffer = pin_.AddrOfPinnedObject().ToPointer(); // assume a pointer to arr also points to first element in array
        array_ = new indirect_smart_ptr<htm::Array>();
        array_->p.reset(new htm::Array(type, buffer, count));
      }
      ~cs_Array() { if (array_) delete array_;  array_ = nullptr; }
      !cs_Array() { if (array_) delete array_;  array_ = nullptr; }
      
      
      property T element[System::UInt64]{
        T get(System::UInt64 i) { 
          if (i >= array_->p->getCount())
              CS_THROW("Index out of range: ");
          void* ptr = array_->p->getBuffer();
          return getManagedValue<T>(ptr, i);
        }
        void set(System::UInt64 i, T value) {
          if (i >= array_->p->getCount())
              CS_THROW("Index out of range: ");
          void* ptr = array_->p->getBuffer();
          setManagedValue(value, ptr, i);
        }
      }


      T operator[] (int x) {
        return arr_[x];
      }

    private:
      indirect_smart_ptr<htm::Array>* array_;  // pointer to the underlining unmanaged Array object
      array<T>^ arr_;          // holds the managed array if there is one.
      GCHandle pin_;           // pins buffer in memory (note: pin_ptr cannot be put on a managed class)
    };
    
    
    //////////////////////////////////////////////////
    ///          ArrayRef                          ///
    //////////////////////////////////////////////////
    // cs_ArrayRef is a wrapper around htm::ArrayRef which are based on ArrayBase
    // An ArrayBase is used for passing arrays of data back and forth between
    // a client application and NuPIC, minimizing copying. It facilitates
    // both zero-copy and one-copy operations.
    // The Nupic Core owns this buffer.  Do not delete.
    ///
    generic <typename T>
    public ref class cs_ArrayRef 
    {
    internal:
      cs_ArrayRef(ArrayRef* arr) {
        array_ = arr;
        if (array_->getType() != parseT(T::typeid)) {
          CS_THROW(("Unexpected Element Type in ArrayRef, Expected " + std::string(htm::BasicType::getName(array_->getType()))).c_str());
        }
      }

    public:
      property System::Int32 count {
        System::Int32 get() {
          return (System::Int32)array_->getCount();
        }
      }
      
      property T element[System::UInt64]{
        T get(System::UInt64 i) { 
          if (i >= array_->getCount())
              CS_THROW("Index out of range: ");
          void* ptr = array_->getBuffer();
          return getManagedValue<T>(ptr, i);
        }
        void set(System::UInt64 i, T value) {
          if (i >= array_->getCount())
              CS_THROW("Index out of range: ");
          void* ptr = array_->getBuffer();
          setManagedValue<T>(value, ptr, i);
        }
      }
      

    private:
      ArrayRef *array_;  // pointer to the underlining ArrayRef object (unmanaged).  This is not deleted.

    };
    





    /////////////////////////////////////////////
    //            Value
    /////////////////////////////////////////////
    /**
    * The Value class is used to store construction parameters
    * for regions and links. A YAML string specified by the user
    * is parsed and converted into a set of Values.
    * So, this wrapper allows read-only access into this set of parameters.
    */
    
 TODO:  The Value class was rewritten. It combined the original Value, ValueMap, and Scalar classes.
        The following needs to be re-written as well.
    
    public ref class cs_Value
    {
    internal:
      cs_Value(htm::Value* value, System::Boolean own) { value_ = value; own_ = own; }
      htm::Value* getUnmanaged() { return value_;  }
    public:
      cs_Value(cs_Scalar^ s) { value_ = new htm::Value(s->getUnmanaged());  own_ = true; }
      template<typename T> cs_Value(cs_Array<T>^ a)  { value_ = new htm::Value(a->getUnmanaged());  own_ = true; }
      cs_Value(String^ s) { value_ = new htm::Value(std::make_shared<std::string>(StringToUTF8(s))); }
      ~cs_Value() { if (own_) delete value_; }
      !cs_Value() { if (own_) delete value_; }

      Boolean isArray()  { return value_->isArray(); }
      Boolean isString() { return value_->isString(); }
      Boolean isScalar() { return value_->isScalar(); }
      String^ getCategory() {
        htm::Value::Category c = value_->getCategory();
        if (c == htm::Value::Category::scalarCategory) return "scalarCategory";
        if (c == htm::Value::Category::arrayCategory)  return "arrayCategory";
        if (c == htm::Value::Category::stringCategory) return "stringCategory";
        return "";
      }


      System::Int32 getType() { return (System::Int32) value_->getType(); }

      template <typename T> T getScalarT() { return value_->getScalarT<T>(); }
      template <typename T> cs_Array<T>^  getArray()  { return gcnew cs_Array<T>(value_->getArray()); }
      String^ getString() { std::shared_ptr<std::string> s = value_->getString(); return UTF8ToString(*s); }

      property cs_Value^ value[String^] {
        cs_Value^ get(String^ key) { CHKEXP(return gcnew cs_Value(&valueMap_->getValue(StringToUTF8(key)), false); )}
        void set(String^ key, cs_Value^ val) { CHKEXP(valueMap_->add(StringToUTF8(key), *val->getUnmanaged());) }
      }
      Boolean contains(String^ key) { return valueMap_->contains(StringToUTF8(key)); }

      template <typename T> T getScalarT(String^ key, T defaultValue) { return valueMap_->getScalarT<T>(StringToUTF8(key), defaultValue); }
      template <typename T> T getScalarT(String^ key) { CHKEXP(return valueMap_->getScalarT<T>(StringToUTF8(key));) }
      cs_Scalar^ getScalar(String^ key) { return gcnew cs_Scalar(valueMap_->getScalar(StringToUTF8(key))); }
      generic <typename T> cs_Array<T>^  getArray(String^ key) { return gcnew cs_Array<T>(valueMap_->getArray(StringToUTF8(key))); }
      String^ getString(String^ key) { boost::shared_ptr<std::string> s = valueMap_->getString(StringToUTF8(key)); return UTF8ToString(*s); }

      String^ getDescription() { return UTF8ToString(value_->getDescription()); }

    private:
      htm::Value *value_;
      System::Boolean own_;
    };


    private:
      htm::ValueMap *valueMap_;
    };



    /////////////////////////////////////////////
    //            Timer
    /////////////////////////////////////////////
    ref class cs_Timer
    {
    internal:
      cs_Timer(htm::Timer* timer, System::Boolean own) { timer_ = timer; own_ = own; }
    public:
      cs_Timer(System::Boolean startme) { timer_ = new Timer(startme); own_ = true; }

      virtual 
      ~cs_Timer() { if (own_) delete timer_; }
      !cs_Timer() { if (own_) delete timer_; }
      void start() { timer_->start(); }
      void stop() { timer_->stop(); }
      System::Double getElapsed() { return timer_->getElapsed(); }
      void reset() { timer_->reset(); }
      System::UInt64 getStartCount() { return timer_->getStartCount(); }
      System::Boolean isStarted() { return timer_->isStarted(); }
      System::String^ toString() { return UTF8ToString(timer_->toString()); }

    private:
      htm::Timer * timer_;
      bool own_;
    };



}   // namespace htm_core_cs

#endif // NTA_CS_UTILS_HPP