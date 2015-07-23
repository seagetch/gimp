/*
 * glib-cxx-impl.hpp
 *
 * Copyright (C) 2015 - seagetch
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __GLIB_CXX_IMPL_HPP__
#define __GLIB_CXX_IMPL_HPP__

#include <glib.h>
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"

namespace GLib {

const char* Key = "cxx";


template<typename Class, typename Ret, typename... Args, Ret (Class::*Callback)(Arg...)>
Ret fn(GObject* obj, Arg... arg) {
  Class* impl;
  impl = reinterpret_cast<Class>(g_object_get_data(obj, Key));
  return (impl->*Callback)(arg...);
};

template<typename InstanceClass>
class Class {
  static GType class_type = G_TYPE_NONE;
public:
  typedef InstanceClass Instance;
   
  static void initialize_class(GObjectClass* klass) {
    Instance::initialize_class(klass);
  };

  static void finalize_class(GObjectClass* klass) {
    Instance::finalize_class(klass);
  }

  static void initialize_instance(GObject* obj, GObjectClass* klass) {
    Instance* instance = new Instance(obj, klass);
  };

  static GType get_type() {
  };
};


class ObjectImpl {
public:
  static void initialize_class(GObjectClass*) {
  };

  static void finalize_class(GObjectClass*) {
  };
  
protected:
  ::GObject* gobject;

public:
  ObjectImpl(::GObject* o, ::GObjectClass* klass) : gobject(o) { 
    g_object_set_cxx_object (gobject, Key, this);
    initialize(klass);
  };

  ~ObjectImpl() { };

  virtual void initialize(GObjectClass* klass);
  virtual void finalize();
  virtual void dispose();
  virtual void set_property(guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec);
  virtual void get_property(guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec);
};

};

#endif