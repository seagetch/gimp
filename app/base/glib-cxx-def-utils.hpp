/*
 * glib-cxx-def-utils.hpp
 *
 *  Created on: 2017/06/25
 *      Author: seagetch
 */

#ifndef APP_BASE_GLIB_CXX_DEF_UTILS_HPP_
#define APP_BASE_GLIB_CXX_DEF_UTILS_HPP_

extern "C" {
#include <gtk/gtk.h>
#include <gtk/gtkbox.h>
#include <gtk/gtkcontainer.h>
#include <gtk/gtkscrolledwindow.h>
}
#include "base/glib-cxx-bridge.hpp"

template<typename T>
class GDefineWrapper : public GWrapper<T>
{
  typedef GWrapper<T> super;
public:
  GDefineWrapper() : super(NULL) { };
  GDefineWrapper(T* object) : super(object) { };
  GDefineWrapper(const GDefineWrapper& src) : super (src.obj) { }

  operator T* () { return (T*)(*(super*)this); };
  template<typename G> operator G* () { return (G*)(*(super*)this); };
  T* operator ->() { return (T*)(*(super*)this); };
  operator bool() { return bool(*(super*)this); };

  void operator = (T* src) { ((super*)this) = src; }
  template<typename Ret, typename C, typename... Args>
  auto operator []( Ret f(C*, Args...) ) { return (*(super*)this)[f]; };
  template<typename Ret, typename C, typename... Args>
  auto operator []( Ret f(const C*, Args...) ) { return (*(super*)this)[f]; };
  GValueWrapper operator[](const gchar* name) { return (*(super*)this)[name]; }

  // Helpers for easy GUI definition.
  template<typename T2>
  void pack_start(T2* child, gboolean expand, gboolean fill, guint padding) {
    (*this)[gtk_box_pack_start](GTK_WIDGET(child), expand, fill, padding);
  }
  template<typename T2, typename F>
  void pack_start(T2* child, gboolean expand, gboolean fill, guint padding, F init) {
    pack_start<T2>(child, expand, fill, padding);
    init(GDefineWrapper<T2>(child));
  }

  template<typename T2>
  void pack_end(T2* child, gboolean expand, gboolean fill, guint padding) {
    (*this)[gtk_box_pack_end](GTK_WIDGET(child), expand, fill, padding);
  }
  template<typename T2, typename F>
  void pack_end(T2* child, gboolean expand, gboolean fill, guint padding, F init) {
    pack_end<T2>(child, expand, fill, padding);
    init(GDefineWrapper<T2>(child));
  }

  template<typename T2>
  void add(T2* child) {
    (*this)[gtk_container_add](GTK_WIDGET(child));
  }
  template<typename T2, typename F>
  void add(T2* child, F init) {
    add<T2>(child);
    init(GDefineWrapper<T2>(child));
  }

  template<typename T2>
  void add_with_viewport(T2* child) {
    (*this)[gtk_scrolled_window_add_with_viewport](GTK_WIDGET(child));
  }
  template<typename T2, typename F>
  void add_with_viewport(T2* child, F init) {
    add_with_viewport<T2>(child);
    init(GDefineWrapper<T2>(child));
  }
};

template<class T, typename F>
inline GDefineWrapper<T> with(T* obj, F init) {
  auto result = GDefineWrapper<T>(obj);
  init(result);
  return result;
}

template<class T, typename F>
inline GDefineWrapper<T> with(GWrapper<T> obj, F init) {
  auto result = GDefineWrapper<T>(obj.ptr());
  init(result);
  return result;
}

template<class T, typename F>
inline GDefineWrapper<T> with(GDefineWrapper<T> obj, F init) {
  init(obj);
  return obj;
}

#endif /* APP_BASE_GLIB_CXX_DEF_UTILS_HPP_ */
