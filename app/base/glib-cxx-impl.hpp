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


template <char const* name, typename CStructs, typename Impl, typename... IFaces>
class GClass {
public:
  static GType get_type();
  typedef typename CStructs::Instance Instance;
  typedef typename CStructs::Class Class;
  typedef GLib::TraitsBase<Instance, Class, &GClass::get_type> Traits;

  static gpointer parent_class;
  static gint     private_offset;

  static void     instance_init     (Instance        *obj) {
    void* ptr = get_private(obj);
    Impl* i = new(ptr) Impl(G_OBJECT(obj));
  }

  static void     instance_finalize (GObject         *obj) {
    Impl* i = get_private(obj);
    i->~Impl();
    G_OBJECT_CLASS(parent_class)->finalize(obj);
  }

  static void     class_intern_init (gpointer klass) {
#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
    parent_class = g_type_class_peek_parent (klass);
    if (private_offset != 0)
      g_type_class_adjust_private_offset (klass, &private_offset);
#else
    parent_class = g_type_class_peek_parent (klass);
#endif /* GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38 */
    Impl::class_init ((Class*) klass);
    G_OBJECT_CLASS(klass)->finalize  = instance_finalize;
  }

  template<typename G>
  static Impl* get_private(G* obj) {
    return G_TYPE_INSTANCE_GET_PRIVATE(obj, Traits::get_type(), Impl);
  };

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
  class with_class {
    gpointer klass;
  public:
    with_class(gpointer class_struct) : klass(class_struct) { };

    template<typename InstanceStruct>
    with_class& as_class(void (*callback)(typename GLib::Traits<InstanceStruct>::Class*)) {
      typename GLib::Traits<InstanceStruct>::Class* casted_class = GLib::Traits<InstanceStruct>::cast_class(klass);
      (*callback) (casted_class);
      return *this;
    }
  };
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

};


#endif
