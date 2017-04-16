/*
 * gtk-cxx-utils.hpp
 *
 *  Created on: 2017/04/09
 *      Author: seagetch
 */

#ifndef APP_BASE_GLIB_CXX_BRIDGE_HPP_
#define APP_BASE_GLIB_CXX_BRIDGE_HPP_

extern "C" {
#ifdef HAVE_GTK_H
#include <gtk.h>
#endif
}
#include "base/glib-cxx-impl.hpp"

namespace GtkCXX {
#ifdef HAVE_GTK_H
template<> inline cast<GtkTreeStore>(gpointer ptr) { return GTK_TREE_STORE(ptr); }
template<> inline cast<GtkLayer>(gpointer ptr) { return GTK_LAYER(ptr); }
template<> inline cast<GtkWidget>(gpointer ptr) { return GTK_WIDGET(ptr); }
#endif
};

#endif /* APP_BASE_GLIB_CXX_BRIDGE_HPP_ */
