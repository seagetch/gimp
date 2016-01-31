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

#include <glib.h>
#include <glib/gprintf.h>
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

class PluginDetatcher {
private:
  GtkWidget* child;
public:
  PluginDetatcher(GtkWidget* child) {
    this->child = child;
  }
  ~PluginDetatcher() {
  }
  void on_destroy(GtkWidget* widget) {
    g_object_ref(child);
    GtkContainer* container = GTK_CONTAINER(gtk_widget_get_parent(child));
    gtk_container_remove(container, child);
  }
};

class GimpImageWindowWebviewPrivate {
private:
  GimpImageWindow* window;
  GtkWidget* force_update_widget(GtkWidget* widget);
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
force_update_widget (GtkWidget* widget) {
  GtkWidget* eventbox = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (eventbox),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(eventbox), widget);
  PluginDetatcher* detatcher = new PluginDetatcher(widget);
  g_object_set_cxx_object(G_OBJECT(eventbox), "_behavior", detatcher);
  g_signal_connect_delegator (G_OBJECT(eventbox), "destroy", 
                              Delegator::delegator(detatcher, &PluginDetatcher::on_destroy));
  gtk_container_check_resize(GTK_CONTAINER(eventbox));
  return eventbox;
}

GtkWidget* 
GimpImageWindowWebviewPrivate::
create_plugin_widget(WebKitWebView* view, 
                     gchar* mime_type,
                     gchar* uri,
                     GHashTable* param)
{
  GtkWidget* result = NULL;
  g_print("WEBVIEW::create_plugin:%s\n", uri);
  if (strcmp(mime_type, "application/gimp") == 0) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
    if (strncmp(uri, "file://", strlen("file://")) == 0)
      uri += strlen("file://");
    if (strcmp(uri, "/toolbar") == 0) {
      g_print("WEBVIEW:toolbar=%lx\n", (gulong)priv->toolbar);
      result = priv->toolbar;
    } else if (strncmp(uri, "/dock", strlen("/dock")) == 0) {
      gchar* dock_name = uri + strlen("/dock");
      if (strcmp(dock_name,"/left") == 0) {
        g_print("WEBVIEW:left_docks=%lx\n", (gulong)priv->left_docks);
        result = priv->left_docks;
      } else if (strcmp(dock_name, "/right") == 0) {
        g_print("WEBVIEW:right_docks=%lx\n", (gulong)priv->right_docks);
        result = priv->right_docks;
      }
    } else if (strcmp(uri, "/images") == 0) {
        g_print("WEBVIEW:images=%lx\n", (gulong)priv->notebook);
      result = priv->notebook;
    } else if (strcmp(uri, "/menubar") == 0) {
        g_print("WEBVIEW:menubar=%lx\n", (gulong)priv->menubar);
      result = priv->menubar;
    } else if (strncmp(uri, "/dialog/", strlen("/dialog/")) == 0) {
      gchar* word_head = uri + strlen("/dialog/");
      gchar* word_term = strstr(word_head,"/");
      int index = word_term ? word_term - word_head: strlen(word_head); 
      StringHolder dialog_name = strndup(word_head, index);
      GimpUIManager* ui_manager = gimp_image_window_get_ui_manager(window);
      result = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
                                               gtk_widget_get_screen (GTK_WIDGET(view)),
                                               ui_manager,
                                               dialog_name.ptr(), -1, TRUE);
      gtk_widget_show(result);
      g_print("WEBVIEW:dialog[%s]%lx\n", dialog_name.ptr(), (gulong)result);
    }
  }
  if (result)
    return force_update_widget(result);
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
  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW(window), NULL);
  g_print("WEBVIEW::create: window=%lx\n", (ulong)window);
  this->window = window;
  result = webkit_web_view_new ();
  // Get file name from config
  StringHolder filename = g_strdup("/tmp/template.html");
  StringHolder file_uri = g_strdup_printf("file://%s", filename.ptr());
  webkit_web_view_load_uri(WEBKIT_WEB_VIEW(result), file_uri.ptr());
  if (result) {
    g_object_set_cxx_object(G_OBJECT(result), "_behavior", this);
    g_signal_connect_delegator (G_OBJECT(result), "create-plugin-widget", 
                                Delegator::delegator(this, &GimpImageWindowWebviewPrivate::create_plugin_widget));
    g_signal_connect_delegator (G_OBJECT(result), "navigation-policy-decision-requested",
                                Delegator::delegator(this, &GimpImageWindowWebviewPrivate::navigation_policy_decision_requested));
  }
  return result;
}


extern "C" {
GtkWidget*
gimp_image_window_get_webview (GimpImageWindow* window)
{
  GimpImageWindowPrivate* win_private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  return win_private->webview;
}


void
gimp_image_window_configure_webview (GimpImageWindow* window)
{
}


GtkWidget*
gimp_image_window_create_webview (GimpImageWindow* window)
{
  GimpImageWindowWebviewPrivate* priv = new GimpImageWindowWebviewPrivate();
  GimpImageWindowPrivate* win_private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  GtkWidget* result = priv->create(window);
  win_private->webview = result;
  return result;
}

}
