/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

extern "C" {

#include "config.h"

#include <gegl.h>
#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "display-types.h"

#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimpprogress.h"
#include "core/gimpcontainer.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdockcolumns.h"
#include "widgets/gimpdockcontainer.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpmenufactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpsessioninfo-aux.h"
#include "widgets/gimpsessionmanaged.h"
#include "widgets/gimpsessioninfo-dock.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpview.h"
#include "widgets/gimptooloptionstoolbar.h"
#include "widgets/gimppopupbutton.h"

#include "gimpdisplay.h"
#include "gimpdisplay-foreach.h"
#include "gimpdisplayshell.h"
#include "gimpdisplayshell-appearance.h"
#include "gimpdisplayshell-close.h"
#include "gimpdisplayshell-scroll.h"
#include "gimpdisplayshell-scale.h"
#include "gimpdisplayshell-tool-events.h"
#include "gimpdisplayshell-transform.h"
#include "gimpimagewindow.h"
#include "gimpimagewindow-private.h"
#include "gimpimagewindow-webview.h"
#include "gimpstatusbar.h"

#include "gimp-log.h"
#include "gimp-intl.h"


#include <webkit/webkit.h>
}

#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"

const static 

class GimpImageWindowWebviewPrivate {
private:
  GimpImageWindow* window;
public:
  GimpImageWindowWebviewPrivate() : window(NULL) { };
  GtkWidget* 
  create_plugin_widget(WebKitWebView* view, 
                       gchar* mime_type,
                       gchar* uri,
                       GHashTable* param);

  gboolean 
  navigation_policy_decision_requested(WebKitWebView* view,
                                       WebKitWebFrame* frame,
                                       WebKitNetworkRequest* req,
                                       WebKitWebNavigationAction* action,
                                       WebKitWebPolicyDecision* decision);

  GtkWidget* create(GimpImageWindow* window);
};

GtkWidget* 
GimpImageWindowWebviewPrivate::
create_plugin_widget(WebKitWebView* view, 
                     gchar* mime_type,
                     gchar* uri,
                     GHashTable* param)
{
  if (strcmp(mime_type, "application/gimp") == 0) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
    if (strcmp(uri, "") == 0) {
      // toolbar
	  priv->toolbar = gimp_tool_options_toolbar_new (priv->gimp,
	      gimp_dialog_factory_get_menu_factory (priv->dialog_factory));
    } else if (strcmp(uri, "") == 0) {
      // left / right docks
      gimp_dock_columns_new (gimp_get_user_context (priv->gimp),
                             priv->dialog_factory,
                             priv->menubar_manager);
    } else if (strcmp(uri, "") == 0) {
      // notebook (contains displayshell)
      priv->notebook = gtk_notebook_new ();
      gtk_notebook_set_scrollable (GTK_NOTEBOOK (priv->notebook), TRUE);
      gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);
      gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
      gtk_notebook_set_tab_pos (GTK_NOTEBOOK (priv->notebook), GTK_POS_LEFT);  
    } else if (strcmp(uri, "") == 0) {
      // menubar
      gtk_ui_manager_get_widget (GTK_UI_MANAGER (priv->menubar_manager),
                                 "/image-menubar");
    }
  }
  return NULL;
}


gboolean
GimpImageWindowWebviewPrivate::
navigation_policy_decision_requested(WebKitWebView* view,
                                     WebKitWebFrame* frame,
                                     WebKitNetworkRequest* req,
                                     WebKitWebNavigationAction* action,
                                     WebKitWebPolicyDecision* decision)
{
  GWrapper<WebKitNetworkRequest> request = req;
  StringHolder uri = g_strdup((const gchar*)request.get("uri"));

  
  
  return FALSE;
}


GtkWidget* 
GimpImageWindowWebviewPrivate::create(GimpImageWindow* window)
{
  GtkWidget* result = NULL;
  result = webkit_web_view_new ();
  // Get file name from config
  StringHolder filename = ;
  StringHolder file_uri = g_sprintf("file://%s", filename.ptr());
  webkit_web_view_load_uri(result, file_uri.ptr());
  if (result)
    g_object_set_cxx_object(G_OBJECT(result), "_behavior", this);
  return result;
}


extern "C" {
GtkWidget*
gimp_image_window_get_webview (GimpImageWindow* window)
{
}


void
gimp_image_window_configure_webview (GimpImageWindow* window)
{
}


GtkWidget*
gimp_image_window_create_webview (GimpImageWindow* window)
{
  GimpImageWindowWebviewPrivate* priv = new GimpImageWindowWebviewPrivate();
  return priv->create(window);
}

}
