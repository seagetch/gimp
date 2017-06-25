#ifndef APP_BASE_GLIB_CXX_BRIDGE_HPP_
#define APP_BASE_GLIB_CXX_BRIDGE_HPP_

#ifdef __cplusplus

extern "C" {
#include <gtk/gtk.h>
}
#include "base/glib-cxx-impl.hpp"

__DECLARE_GTK_CAST__(GtkTreeStore, GTK_TREE_STORE, gtk_tree_store);
__DECLARE_GTK_CAST__(GtkWidget, GTK_WIDGET, gtk_widget);
__DECLARE_GTK_CAST__(GtkContainer, GTK_CONTAINER, gtk_container);
__DECLARE_GTK_CAST__(GtkBox, GTK_BOX, gtk_box);
__DECLARE_GTK_CAST__(GtkScrolledWindow, GTK_SCROLLED_WINDOW, gtk_scrolled_window);

#endif /* __cplusplus */

#endif /* APP_BASE_GLIB_CXX_BRIDGE_HPP_ */
