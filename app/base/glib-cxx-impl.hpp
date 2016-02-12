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
#include <vector>
#include <stdexcept>
#include <memory>
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"

namespace GLib {

/////////////////////////////////////////////////////////////////////////////

template<class Class>
struct GTypeTraits { static GType get_type() { return G_TYPE_OBJECT; }; };
template<> struct GTypeTraits<bool> { static GType get_type() { return G_TYPE_BOOLEAN; }; };
template<> struct GTypeTraits<char> { static GType get_type() { return G_TYPE_CHAR; }; };
template<> struct GTypeTraits<unsigned char> { static GType get_type() { return G_TYPE_UCHAR; }; };
template<> struct GTypeTraits<int> { static GType get_type() { return G_TYPE_INT; }; };
template<> struct GTypeTraits<unsigned int> { static GType get_type() { return G_TYPE_UINT; }; };
template<> struct GTypeTraits<long> { static GType get_type() { return G_TYPE_LONG; }; };
template<> struct GTypeTraits<unsigned long> { static GType get_type() { return G_TYPE_ULONG; }; };
template<> struct GTypeTraits<unsigned char*> { static GType get_type() { return G_TYPE_STRING; }; };
template<> struct GTypeTraits<float> { static GType get_type() { return G_TYPE_FLOAT; }; };
template<> struct GTypeTraits<double> { static GType get_type() { return G_TYPE_DOUBLE; }; };
template<> struct GTypeTraits<void> { static GType get_type() { return G_TYPE_NONE; }; };

/////////////////////////////////////////////////////////////////////////////
template<typename ParentInstance>
struct GInstance {
  typedef ParentInstance parent;
  parent parent_instance;
  gpointer cxx_instance;
};

template<typename ParentClass>
struct GClass {
  typedef ParentClass parent;
  parent parent_class;
  gpointer cxx_class;
};

/////////////////////////////////////////////////////////////////////////////

template<class T>
T* get_singleton() {
  static volatile T* instance__volatile = NULL;
  if (g_once_init_enter (&instance__volatile)) {
    T* instance = new T();
    g_once_init_leave (&instance__volatile, instance);
  }
  return (T*)instance__volatile;
}


template<typename ParentClass, typename ParentInstance>
class CXXInstance {
public:
  typedef GInstance<ParentInstance> glib_instance;
  typedef GClass<ParentClass> glib_class;
protected:
  glib_instance* _gobj;
public:
  CXXInstance() { };
  virtual ~CXXInstance() {};
  virtual void finalize() {};
  virtual void dispose() {};
  glib_instance* gobj() { return _gobj; };
  void gobj(glib_instance* ins) { _gobj = ins; };
};


template<class CXXInstance>
class CXXClass {
protected:
  typedef CXXInstance cxx_instance;
  typedef typename CXXInstance::glib_class glib_class;
  typedef typename CXXInstance::glib_instance glib_instance;
  typedef CXXClass cxx_class;
  glib_class* gclass;
  gpointer    _parent_class;
  gsize g_define_type_id__volatile;
  GType (*accessor)();
  const gchar* classname;
  
  struct PropertyHandler {
    virtual ~PropertyHandler() {}
    virtual bool can_get() = 0;
    virtual bool can_set() = 0;
    virtual void get(cxx_instance* instance, GValue *value) = 0;
    virtual void set(cxx_instance* instance, const GValue *value) = 0;
  };

  #define DECLARE_ACCESSOR(Target, type) \
    Target (cxx_instance::*getter)(); \
    void (cxx_instance::*setter)(const Target value); \
    bool can_get() { return getter != 0; }; \
    bool can_set() { return setter != 0; }; \
    virtual void get(cxx_instance* instance, GValue* value) { \
      g_print("PropertyHandler::get\n"); \
      g_value_set_##type(value, (instance->*getter)()); \
    }; \
    virtual void set(cxx_instance* instance, const GValue* value) { \
      g_print("PropertyHandler::set\n"); \
      (instance->*setter)(g_value_get_##type(value)); \
    };

  #define DECLARE_SPECIAL_ACCESSOR(Target, type) \
  template<typename Dummy> struct TypedPropertyHandler<Target, Dummy> : public PropertyHandler {\
    typedef Dummy ret; \
    DECLARE_ACCESSOR(Target, type) \
  }

  template<typename T, typename Dummy=int>
  struct TypedPropertyHandler : public PropertyHandler {
    typedef Dummy ret;
    DECLARE_ACCESSOR(T, object)
  };
  DECLARE_SPECIAL_ACCESSOR(bool, boolean);
  DECLARE_SPECIAL_ACCESSOR(char, schar);
  DECLARE_SPECIAL_ACCESSOR(guchar, uchar);
  DECLARE_SPECIAL_ACCESSOR(int, int);
  DECLARE_SPECIAL_ACCESSOR(guint, uint);
  DECLARE_SPECIAL_ACCESSOR(long, long);
  DECLARE_SPECIAL_ACCESSOR(gulong, ulong);
  DECLARE_SPECIAL_ACCESSOR(float, float);
  DECLARE_SPECIAL_ACCESSOR(double, double);
  DECLARE_SPECIAL_ACCESSOR(guchar*, string);

  #undef DECLARE_ACCESSOR
  #undef DECLARE_SPECIAL_ACCESSOR

  std::vector<guint> signals;
  std::vector<PropertyHandler*> properties;

  template<typename Ret, Ret(cxx_instance::*func_ptr)()>
  static Ret _wrap(cxx_instance* instance) {
    return (instance->*func_ptr)();
  };

  template<void(cxx_instance::*func_ptr)()>
  static void _wrap(cxx_instance* instance) {
    (instance->*func_ptr)();
  };

  template<typename Ret, typename A1, Ret(cxx_instance::*func_ptr)(A1)>
  static Ret _wrap(cxx_instance* instance, A1 a1) {
    return (instance->*func_ptr)(a1);
  };

  template<typename A1, void(cxx_instance::*func_ptr)(A1)>
  static void _wrap(cxx_instance* instance, A1 a1) {
    (instance->*func_ptr)(a1);
  };

  template<typename Ret, typename A1, typename A2, Ret(cxx_instance::*func_ptr)(A1, A2)>
  static Ret _wrap(cxx_instance* instance, A1 a1, A2 a2) {
    return (instance->*func_ptr)(a1, a2);
  };

  template<typename A1, typename A2, void(cxx_instance::*func_ptr)(A1, A2)>
  static void _wrap(cxx_instance* instance, A1 a1, A2 a2) {
    (instance->*func_ptr)(a1, a2);
  };

  template<typename Ret, typename A1, typename A2, typename A3, Ret(cxx_instance::*func_ptr)(A1, A2, A3)>
  static Ret _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3) {
    return (instance->*func_ptr)(a1, a2, a3);
  };

  template<typename A1, typename A2, typename A3, void(cxx_instance::*func_ptr)(A1, A2, A3)>
  static void _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3) {
    (instance->*func_ptr)(a1, a2, a3);
  };

  template<typename Ret, typename A1, typename A2, typename A3, typename A4, Ret(cxx_instance::*func_ptr)(A1, A2, A3, A4)>
  static Ret _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3, A4 a4) {
    (instance->*func_ptr)(a1, a2, a3, a4);
  };

  template<typename A1, typename A2, typename A3, typename A4, void(cxx_instance::*func_ptr)(A1, A2, A3, A4)>
  static void _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3, A4 a4) {
    (instance->*func_ptr)(a1, a2, a3, a4);
  };

  template<typename Ret, typename A1, typename A2, typename A3, typename A4, typename A5, Ret(cxx_instance::*func_ptr)(A1, A2, A3, A4, A5)>
  static Ret _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    (instance->*func_ptr)(a1, a2, a3, a4, a5);
  };

  template<typename A1, typename A2, typename A3, typename A4, typename A5, void(cxx_instance::*func_ptr)(A1, A2, A3, A4, A5)>
  static void _wrap(cxx_instance* instance, A1 a1, A2 a2, A3 a3, A4 a4, A5 a5) {
    (instance->*func_ptr)(a1, a2, a3, a4, a5);
  };
  
  template<typename Dst, typename Src>
  static void _bind(Dst& dst, Src src) {
    dst = reinterpret_cast<Dst>(src);
  }

  template<typename Ret>
  void _signal(const gchar* signal_name, guint flags = G_SIGNAL_RUN_FIRST) {
    g_signal_new (signal_name, G_TYPE_FROM_CLASS (gclass),
                  (GSignalFlags)flags, 0, NULL, NULL,
                  NULL, GTypeTraits<Ret>::get_type(), 0);
  }

  template<typename Ret, typename... As>
  void _signal(const gchar* signal_name, guint flags = G_SIGNAL_RUN_FIRST) {
    g_signal_new (signal_name, G_TYPE_FROM_CLASS (gclass),
                  (GSignalFlags)flags, 0, NULL, NULL,
                  NULL, GTypeTraits<Ret>::get_type(), sizeof...(As), GTypeTraits<As...>::get_type());
  }
  
  static void 
  _get_property (glib_instance*   obj,
                 guint      property_id,
                 GValue     *value,
                 GParamSpec *pspec) {
    g_print("Getter: name = %d\n", property_id);
    PropertyHandler* handler = NULL;
    try {
      glib_class*   klass    = G_TYPE_INSTANCE_GET_CLASS(obj, G_TYPE_FROM_INSTANCE(obj), glib_class);
      cxx_class*    cxxclass = (cxx_class*)(klass->cxx_class);
      cxx_instance* instance = (cxx_instance*)(obj->cxx_instance);
      handler = cxxclass->properties.at(property_id - 1);
      if (handler && handler->can_get())
        handler->get(instance, value);
    } catch(std::out_of_range& e) {
    }
  };
  
  static void 
  _set_property (glib_instance*      obj,
                 guint         property_id,
                 const GValue* value,
                 GParamSpec*   pspec) {
    g_print("Setter: name = %d\n", property_id);
    PropertyHandler* handler = NULL;
    try {
      glib_class*   klass    = G_TYPE_INSTANCE_GET_CLASS(obj, G_TYPE_FROM_INSTANCE(obj), glib_class);
      cxx_class*    cxxclass = (cxx_class*)(klass->cxx_class);
      cxx_instance* instance = (cxx_instance*)(obj->cxx_instance);
      handler = cxxclass->properties.at(property_id - 1);
      if (handler && handler->can_set())
        handler->set(instance, value);
    } catch(std::out_of_range& e) {
    }
  };
  
  template<typename T>
  static GParamSpec* param_spec(const gchar* name, guint flags) {
    return g_param_spec_object(name, NULL, NULL, GTypeTraits<T>::get_type(), (GParamFlags)flags);
  }
  static GParamSpec* param_spec(const gchar* name, guint flags, bool default_value) {
    return g_param_spec_boolean(name, NULL, NULL, default_value, (GParamFlags)flags);
  }
  #define DECLARE_PARAM_SPEC(Target, type) \
  static GParamSpec* param_spec(const gchar* name, guint flags, Target min, Target max, Target default_value) { \
    return g_param_spec_##type(name, NULL, NULL, min, max, default_value, (GParamFlags)flags); \
  }
  DECLARE_PARAM_SPEC(char, char);
  DECLARE_PARAM_SPEC(guchar, uchar);
  DECLARE_PARAM_SPEC(int, int);
  DECLARE_PARAM_SPEC(guint, uint);
  DECLARE_PARAM_SPEC(long, long);
  DECLARE_PARAM_SPEC(gulong, ulong);
  DECLARE_PARAM_SPEC(float, float);
  DECLARE_PARAM_SPEC(double, double);
  
  template<typename T, typename... Args>
  void _property(const gchar* name, 
                T (cxx_instance::*getter)(), 
                void (cxx_instance::*setter)(const T value), Args... args) {
    g_print("REGISTER_PROPERTY: name = %s, type=%s\n", name, g_type_name(GTypeTraits<T>::get_type()));
    unsigned int flags = G_PARAM_CONSTRUCT;
    auto handler = new TypedPropertyHandler<T>();
    handler->getter = getter;
    handler->setter = setter;

    if (getter)
      flags |= G_PARAM_READABLE;

    if (setter)
      flags |= G_PARAM_WRITABLE;

    g_object_class_install_property(G_OBJECT_CLASS(gclass), properties.size() + 1,
      param_spec(name, flags, args...));
//      g_param_spec_object(name, NULL, NULL, GTypeTraits<T>::get_type(), (GParamFlags)flags));

    properties.push_back(handler);
  }
  
public:

  CXXClass(GType (*acc)(), const gchar* name) {
    g_define_type_id__volatile = 0;
    accessor = acc;
    classname = name;
  }
  
  virtual void init() {
    _parent_class = g_type_class_peek_parent (gclass);
    GObjectClass      *object_class   = G_OBJECT_CLASS (gclass);
    _bind(object_class->finalize,     _wrap<&cxx_instance::finalize>);
    _bind(object_class->dispose ,     _wrap<&cxx_instance::dispose>);
    _bind(object_class->get_property, _get_property);
    _bind(object_class->set_property, _set_property);
  };
  
  virtual ~CXXClass() {
  };
  
  gpointer get_parent() { return _parent_class; }

  template<class cxx_instance>
  static void     instance_init(glib_instance *self) {
    cxx_instance* impl = new cxx_instance();
    self->cxx_instance = impl;
    impl->gobj(self);
  };

  template<class cxx_class>
  static void     class_intern_init (gpointer klass) {
    glib_class* gclass = (glib_class*)klass;
    cxx_class* cclass = get_singleton<cxx_class>();

    gclass->cxx_class = cclass;
    cclass->gclass = gclass;
    cclass->init();
  };
  
  virtual glib_instance* cast(gpointer ptr) {
    G_TYPE_CHECK_INSTANCE_CAST((ptr), (get_type()), glib_instance );
  }
  
  virtual glib_class* class_cast(gpointer ptr) {
    G_TYPE_CHECK_CLASS_CAST((ptr), (get_type()), glib_class );
  }
  
  virtual bool is_instance(gpointer i) {
    return G_TYPE_CHECK_INSTANCE_TYPE(i, (get_type()) );
  };
  
  virtual bool is_class(gpointer i) {
    return G_TYPE_CHECK_CLASS_TYPE(i, (get_type()) );
  };

  virtual glib_class* get_class_for(gpointer i) {
    G_TYPE_INSTANCE_GET_CLASS (i, get_type(), glib_class);
  }

  virtual GType get_type () { return G_TYPE_NONE; }

  template<typename glib_class, typename glib_instance, typename cxx_class, typename cxx_instance>
  GType get_type_for() {
    if (g_once_init_enter (&g_define_type_id__volatile)) {
			GType g_define_type_id =
				g_type_register_static_simple ((*accessor)(),
				                               g_intern_static_string (classname),
				                               sizeof (glib_class),
				                               (GClassInitFunc) &CXXClass::class_intern_init<cxx_class>,
				                               sizeof (glib_instance),
				                               (GInstanceInitFunc) &CXXClass::instance_init<cxx_instance>,
				                               (GTypeFlags)0);
      {
//        const GInterfaceInfo g_implement_interface_info = {
//          (GInterfaceInitFunc) gtk_gadget_gizmo_init
//        };
//        g_type_add_interface_static (g_define_type_id, TYPE_GIZMO, &g_implement_interface_info);
      }
      g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
      g_print("REGISTER CXX Type: %s\n", g_type_name(g_define_type_id__volatile));
    }
    g_print("get_type: %s\n", g_type_name(g_define_type_id__volatile));
    return g_define_type_id__volatile;
  };

  
  virtual glib_instance* create() {
    return (glib_instance*)g_object_new(get_type(), NULL);
  };

};

/////////////////////////////////////////////////////////////////////////////

};
#endif
