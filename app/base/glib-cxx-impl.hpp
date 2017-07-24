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

template <typename _ParentTraits>
class ClassExtension {
public:
  typedef _ParentTraits ParentTraits;
  struct Instance {
    typename ParentTraits::Instance parent_instance;
  };
  struct Class {
    typename ParentTraits::Class parent_class;
  };
};

template <typename _ParentTraits, typename _Instance>//, typename _Class = typename std::remove_reference<decltype(*GLib::g_class_type<_Instance>(NULL))>::type>
class ClassHolder {
public:
  using ParentTraits = _ParentTraits;
  using Instance     = _Instance;
  using Class        = typename std::remove_reference<decltype(*GLib::g_class_type<_Instance>(NULL))>::type;
};



template <char const* name, typename Extension, typename Impl, typename... IFaces>
class ClassDefinition {
public:
  static GType get_type();
  typedef typename Extension::Instance Instance;
  typedef typename Extension::Class Class;
  typedef GLib::Traits<Instance, Class, &ClassDefinition::get_type> Traits;

  static void     instance_init     (Instance        *obj) {
    void* ptr = get_private(obj);
    Impl* i = new(ptr) Impl(G_OBJECT(obj));
    i->init();
  }
  static gpointer parent_class;
  static gint     private_offset;

#if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38
  static void     class_intern_init (gpointer klass) {
    parent_class = g_type_class_peek_parent (klass);
    if (private_offset != 0)
      g_type_class_adjust_private_offset (klass, &private_offset);
    Impl::class_init ((Class*) klass);
  }
#else
  static void     class_intern_init (gpointer klass) {
    parent_class = g_type_class_peek_parent (klass);
    Impl::class_init ((Class*) klass);
  }
#endif /* GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_38 */

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

    void clear() { *store = NULL; }
  };
  template<typename Ret, typename G, typename... Args>
  static Binder<Ret, G, Args...> __(Ret (**ptr)(G*, Args...)) { return Binder<Ret, G, Args...>(ptr); }

private:
  struct __Dummy {};
  template <typename D>
  static void add_interface_iter(GType) {};

  template<typename D, typename IFaceTraits, typename... Rest>
  static void add_interface_iter(GType g_define_type_id) {
    void (*init)(typename IFaceTraits::Class* klass);
    init = &Impl::iface_init;
    const GInterfaceInfo g_implement_interface_info = {
      (GInterfaceInitFunc) init, NULL, NULL
    };
    g_type_add_interface_static (g_define_type_id, IFaceTraits::get_type(), &g_implement_interface_info);
    add_interface_iter<D, Rest...>(g_define_type_id);
  }
};


template <char const* name, typename Extension, typename Impl, typename... IFaces>
gpointer
ClassDefinition<name, Extension, Impl, IFaces...>::parent_class = NULL;

template <char const* name, typename Extension, typename Impl, typename... IFaces>
gint
ClassDefinition<name, Extension, Impl, IFaces...>::private_offset = 0;

template <char const* name, typename Extension, typename Impl, typename... IFaces>
GType ClassDefinition<name, Extension, Impl, IFaces...>::get_type (void) {
  static volatile gsize g_define_type_id__volatile = 0;
  if (g_once_init_enter (&g_define_type_id__volatile)) {
    GType g_define_type_id =
        g_type_register_static_simple (Extension::ParentTraits::get_type(),
                                       g_intern_static_string (name),
                                       sizeof (Class),
                                       (GClassInitFunc) class_intern_init,
                                       sizeof (Instance),
                                       (GInstanceInitFunc) &ClassDefinition<name, Extension, Impl, IFaces...>::instance_init,
                                       (GTypeFlags) 0/*flags*/);
    { /* custom code follows */
      //#define G_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init)
      add_interface_iter<__Dummy, IFaces...>(g_define_type_id);
      //#define G_ADD_PRIVATE(TypeName)
      private_offset =
        g_type_add_instance_private (g_define_type_id, sizeof (Impl));
    }
    g_once_init_leave (&g_define_type_id__volatile, g_define_type_id);
  }
  return g_define_type_id__volatile;
}


class ImplBase {
public:
  void* operator new(size_t s, void* mem) { return mem; };
protected:
  GObject* g_object;
public:
  ImplBase(GObject* obj) : g_object(obj) { };
  virtual ~ImplBase() {};
  template<typename Class, typename Impl>
  static void class_init(Class* klass) {
  }
  template<typename IFace>
  IFace* cast() { return IFace::cast(g_object); }
};

};


#endif
