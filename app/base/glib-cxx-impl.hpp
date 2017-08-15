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

#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-utils.hpp"
#include <stdarg.h>
#include <functional>

namespace GLib {

template <typename _Parent>
class DerivedFrom {
public:
  using ParentTraits = Traits<_Parent>;
  struct Instance {
    typename ParentTraits::Instance parent_instance;
  };
  struct Class {
    typename ParentTraits::Class parent_class;
  };
};

template <typename _Parent, typename _Instance>//, typename _Class = typename std::remove_reference<decltype(*GLib::g_class_type<_Instance>(NULL))>::type>
class UseCStructs {
public:
  using ParentTraits = Traits<_Parent>;
  using Instance     = _Instance;
  using Class        = typename std::remove_reference<decltype(*GLib::g_class_type<_Instance>(NULL))>::type;
};


class IGClass {
public:
  class IWithClass {
  protected:
    gpointer klass;
  public:
    IWithClass() : klass(NULL) {}
    ~IWithClass() { }
    template<typename InstanceStruct>
    IWithClass* as_class(std::function<void (typename GLib::Traits<InstanceStruct>::Class*)> callback) {
      typename GLib::Traits<InstanceStruct>::Class* casted_class = GLib::Traits<InstanceStruct>::cast_class(klass);
      callback(casted_class);
      return this;
    }
    virtual IWithClass* install_property(GParamSpec* spec, std::function<Value(GObject*)> getter, std::function<void(GObject*, Value)> setter) = 0;
    virtual IWithClass* install_signal_v(const gchar* name, GType R, size_t size, GType* args) = 0;
    template<typename... Args>
    IWithClass* install_signal(const gchar* name, GType R, Args... args) {
      size_t size = sizeof...(Args);
      GType array[size] = {args...};
      install_signal_v(name, R, size, array);
    }
  };

  virtual GType       type() = 0;
  virtual IWithClass* with_class(gpointer klass) = 0;

  template<typename T>
  struct Range {
    T min;
    T max;
    Range(T _min, T _max) : min(_min), max(_max) {};
  };

  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<int> range, int default_value, GParamFlags flags) {
    return g_param_spec_int(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<uint> range, uint default_value, GParamFlags flags) {
    return g_param_spec_uint(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<long> range, long default_value, GParamFlags flags) {
    return g_param_spec_long(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<ulong> range, ulong default_value, GParamFlags flags) {
    return g_param_spec_ulong(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<float> range, float default_value, GParamFlags flags) {
    return g_param_spec_float(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, Range<double> range, double default_value, GParamFlags flags) {
    return g_param_spec_double(param_name, NULL, NULL, range.min, range.max, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, bool default_value, GParamFlags flags) {
    return g_param_spec_boolean(param_name, NULL, NULL, default_value, flags);
  }
  static GParamSpec* g_param_spec_new(const gchar* param_name, const gchar* default_value, GParamFlags flags) {
    return g_param_spec_string(param_name, NULL, NULL, default_value, flags);
  }

};


template <char const* name, typename CStructs, typename Impl, typename... IFaces>
class GClass : public IGClass {
protected:
  static Array _signals;
  static guint signals_base;
  static Array _properties;
  static guint properties_base;
public:
  static GType get_type();
  virtual GType type() { return GClass::get_type(); }

  using Instance = typename CStructs::Instance;
  using Class    = typename CStructs::Class;
  using Traits   = GLib::TraitsBase<Instance, Class, &GClass::get_type>;

  static gpointer parent_class;
  static gint     private_offset;

  struct Property {
    guint prop_id;
    std::function<Value(GObject*)> getter;
    std::function<void(GObject*, Value)> setter;

    Property(guint id,
        std::function<Value(GObject*)> g,
        std::function<void(GObject*, Value)> s)
    : prop_id(id), getter(g), setter(s) { };
  };

  static IArray<guint> signals() {
    if (!_signals) {
      _signals = hold(g_array_new(FALSE, TRUE, sizeof(guint)));
    }
    return ref<guint>(_signals);
  }
  static IArray<Property> properties() {
    if (!_properties) {
      _properties = hold(g_array_new(FALSE, TRUE, sizeof(Property)));
    }
    return ref<Property>(_properties);
  }

  static void     instance_init     (Instance        *obj) {
    void* ptr = get_private(obj);
    Impl* i = new(ptr) Impl(G_OBJECT(obj));
  }

  static void     instance_finalize (GObject         *obj) {
    Impl* i = get_private(obj);
    i->~Impl();
    G_OBJECT_CLASS(parent_class)->finalize(obj);
  }

  static void set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
  {
//    g_print("GClass:set_property: %u\n", property_id);
    auto properties = GClass::properties();
    if (property_id - GClass::properties_base < properties.size()) {
      Property prop = properties[property_id - GClass::properties_base];
      if (prop.setter)
        prop.setter(object, value);
      else;
    } else {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
  }

  static void get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
  {
//    g_print("GClass:get_property: %u\n", property_id);
    auto properties = GClass::properties();
    if (property_id - GClass::properties_base < properties.size()) {
      Property prop = properties[property_id - GClass::properties_base];
      if (prop.getter) {
        Value get_value = prop.getter(object);
        g_value_copy(get_value, value);
      } else ;
    } else {
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    }
  }

  template<typename G>
  static Impl* get_private(G* obj) {
    return G_TYPE_INSTANCE_GET_PRIVATE(obj, Traits::get_type(), Impl);
  };

  static void     class_intern_init (gpointer klass) {

#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
    parent_class = g_type_class_peek_parent (klass);
    if (private_offset != 0)
      g_type_class_adjust_private_offset (klass, &private_offset);
#else
    parent_class = g_type_class_peek_parent (klass);
#endif /* GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38 */

    G_OBJECT_CLASS(klass)->set_property = set_property;
    G_OBJECT_CLASS(klass)->get_property = get_property;

    Impl::class_init ((Class*) klass);

    G_OBJECT_CLASS(klass)->finalize  = instance_finalize;
  }


  template<typename Ret, typename G, typename... Args>
  struct Binder {
    typedef Ret (*FuncStore)(G*, Args...);
    FuncStore* store;
    Binder(FuncStore* s) : store(s) {};

    template<Ret (Impl::*signature)(Args... args)>
    static Ret callback(G* obj, Args... args) {
      Impl* priv = get_private(obj);
      (priv->*signature)(args...);
    };

    template<Ret (Impl::*signature)(Args... args)>
    void bind() {
      *store = &(callback<signature>);
    };

    template<typename R2, typename G2>
    void bind(R2 (*func)(G2*, Args...)) {
      *store = reinterpret_cast<Ret (*)(G*, Args...)>(func);
    }

    template<typename R2, typename G2>
    void operator = (R2 (*func)(G2*, Args...)) {
      bind(func);
    }

    void clear() { *store = NULL; }
  };
  template<typename Ret, typename G, typename... Args>
  static Binder<Ret, G, Args...> __(Ret (**ptr)(G*, Args...)) { return Binder<Ret, G, Args...>(ptr); }


  class WithClass : public IWithClass {
  public:
    WithClass() : IWithClass() { };

    struct InvalidClass {
      gpointer should_be;
      gpointer passed;
      InvalidClass(gpointer s, gpointer p) : should_be(s), passed(p) { };
    };

    void init(gpointer class_struct) {
      if (!klass)
        klass = class_struct;
      else if (klass != class_struct)
        throw new InvalidClass(klass, class_struct);
    }

    virtual IWithClass* install_property(GParamSpec* spec, std::function<Value(GObject*)> getter, std::function<void(GObject*, Value)> setter) {
      as_class<GObject>([&](GObjectClass* klass) {
        auto properties = GClass::properties();
        guint index = properties.size() + GClass::properties_base;
        g_object_class_install_property (klass, index, spec);
//        g_print("Installed::%s-> prop=%s, id=%u\n", name, g_param_spec_get_name(spec), index);

        properties.append(Property(index, getter, setter));
        Property prop = properties[properties.size() - 1];
      });
      return this;
    }

    virtual IWithClass* install_signal_v(const gchar* signal_name, GType R, size_t size, GType* args) {
      as_class<GObject>([&](GObjectClass* klass) {
        auto signals = GClass::signals();
        guint sig_id = g_signal_newv(signal_name, G_OBJECT_CLASS_TYPE(klass), G_SIGNAL_RUN_LAST, 0 /*Offset*/, NULL, NULL, NULL/*g_closure_marshaller_generic*/, R, size, args);
        signals.append(sig_id);
      });
      return this;
    }

  };

  static WithClass _with_class;

  virtual IWithClass* with_class(gpointer klass) { _with_class.init(klass); return &_with_class; }

private:
  struct __Dummy {};
  template <typename D>
  static void add_interface_iter(GType) {};

  template<typename D, typename IFace, typename... Rest>
  static void add_interface_iter(GType g_define_type_id) {
    using IFaceTraits = GLib::Traits<IFace>;
    void (*init)(typename IFaceTraits::Class* klass);
    init = &Impl::iface_init;
    const GInterfaceInfo g_implement_interface_info = {
      (GInterfaceInitFunc) init, NULL, NULL
    };
    g_type_add_interface_static (g_define_type_id, IFaceTraits::get_type(), &g_implement_interface_info);
    add_interface_iter<D, Rest...>(g_define_type_id);
  }
};


template <char const* name, typename CStructs, typename Impl, typename... IFaces>
gpointer GClass<name, CStructs, Impl, IFaces...>::parent_class = NULL;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
gint GClass<name, CStructs, Impl, IFaces...>::private_offset = 0;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
Array GClass<name, CStructs, Impl, IFaces...>::_signals;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
guint GClass<name, CStructs, Impl, IFaces...>::signals_base = 0;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
Array GClass<name, CStructs, Impl, IFaces...>::_properties;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
guint GClass<name, CStructs, Impl, IFaces...>::properties_base = 1;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
typename GClass<name, CStructs, Impl, IFaces...>::WithClass GClass<name, CStructs, Impl, IFaces...>::_with_class;

template <char const* name, typename CStructs, typename Impl, typename... IFaces>
inline GType GClass<name, CStructs, Impl, IFaces...>::get_type (void) {
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    GType g_define_type_id =
        g_type_register_static_simple (CStructs::ParentTraits::get_type(),
                                       g_intern_static_string (name),
                                       sizeof (Class),
                                       (GClassInitFunc) class_intern_init,
                                       sizeof (Instance),
                                       (GInstanceInitFunc) &GClass<name, CStructs, Impl, IFaces...>::instance_init,
                                       (GTypeFlags) 0/*flags*/);
    add_interface_iter<__Dummy, IFaces...>(g_define_type_id);
    private_offset = g_type_add_instance_private (g_define_type_id, sizeof (Impl));

    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}


class ImplBase {
protected:

public:
  void* operator new(size_t s, void* mem) { return mem; };
protected:
  GObject* g_object;
public:
  ImplBase(GObject* obj) : g_object(obj) { };
  virtual ~ImplBase() { };
  template<typename Class, typename Impl>
  static void class_init(Class* klass) {
  }
  template<typename IFace>
  IFace* cast() { return IFace::cast(g_object); }
};

}; // namespace GLib


#endif
