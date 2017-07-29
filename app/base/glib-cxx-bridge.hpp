#ifndef APP_BASE_GLIB_CXX_BRIDGE_HPP_
#define APP_BASE_GLIB_CXX_BRIDGE_HPP_

#ifdef __cplusplus

extern "C" {
#include <gdk/gdk.h>
#include <gtk/gtk.h>
}
#include "base/glib-cxx-types.hpp"

__DECLARE_GTK_CAST__(GtkTreeStore, GTK_TREE_STORE, gtk_tree_store);
__DECLARE_GTK_CAST__(GtkWidget, GTK_WIDGET, gtk_widget);
__DECLARE_GTK_CAST__(GtkContainer, GTK_CONTAINER, gtk_container);
__DECLARE_GTK_CAST__(GtkBox, GTK_BOX, gtk_box);
__DECLARE_GTK_CAST__(GtkScrolledWindow, GTK_SCROLLED_WINDOW, gtk_scrolled_window);
__DECLARE_GTK_CAST__(GtkMisc, GTK_MISC, gtk_misc);
__DECLARE_GTK_CAST__(GtkWindow, GTK_WINDOW, gtk_window);
__DECLARE_GTK_IFACE__(GtkTreeModel, gtk_tree_model);
__DECLARE_GTK_CLASS__(GtkAdjustment, GTK_TYPE_ADJUSTMENT);
__DECLARE_GTK_CLASS__(GtkTreeView, GTK_TYPE_TREE_VIEW);
__DECLARE_GTK_CLASS__(GtkTreeSelection, GTK_TYPE_TREE_SELECTION);
__DECLARE_GTK_CLASS__(GtkFrame, GTK_TYPE_FRAME);
__DECLARE_GTK_CLASS__(GdkDrawable, GDK_TYPE_DRAWABLE);
__DECLARE_GTK_CLASS__(GdkScreen, GDK_TYPE_SCREEN);
__DECLARE_GTK_CLASS__(GtkLabel, GTK_TYPE_LABEL);
__DECLARE_GTK_CLASS__(GtkToggleButton, GTK_TYPE_TOGGLE_BUTTON);
__DECLARE_GTK_CLASS__(GtkCellRenderer, GTK_TYPE_CELL_RENDERER);

#endif /* __cplusplus */

#endif /* APP_BASE_GLIB_CXX_BRIDGE_HPP_ */
