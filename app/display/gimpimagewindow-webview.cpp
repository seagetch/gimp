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
#include "widgets/gimpdockable.h"
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

extern "C" {
static GimpContext* image_window_get_context(GimpImageWindow* window);
}

class ActiveShellConfigurator {
private:
  CXXPointer<Delegator::Connection> handler;
  GimpImageWindow* window;
public:
  ActiveShellConfigurator(GimpImageWindow* win, GtkWidget* widget) {
    auto dec = decorator(widget, this);
    handler = dec.connecth("expose-event", &ActiveShellConfigurator::expose);
    window = win; 
  }
  void expose(GtkWidget* widget, GdkEvent* ev) {
    g_print("EXPOSE!!!!!\n");
    auto active_shell = _G(window)[gimp_image_window_get_active_shell]();
    _G(window)[gimp_image_window_remove_shell](active_shell);
    _G(window)[gimp_image_window_add_shell](active_shell);
    handler->disconnect();
    //FIXME: should be detached from widget!
    delete this;
  }
};

class DockableWrapper {
private:
  GimpDockable* child;
public:
  DockableWrapper(GimpDockable* c) : child(c) {}
  ~DockableWrapper() {}
  void on_destroy(GtkWidget* widget) {
    gimp_dockable_set_context(GIMP_DOCKABLE(child), NULL);
  }
};

class PluginDetacher {
private:
  GtkWidget* child;
public:
  PluginDetacher(GtkWidget* c) : child(c) {}
  ~PluginDetacher() {}
  void on_destroy(GtkWidget* widget) {
    g_object_ref(child);
    GtkContainer* container = GTK_CONTAINER(gtk_widget_get_parent(child));
    gtk_container_remove(container, child);
  }
};

class GimpImageWindowWebviewPrivate {
private:
  GimpImageWindow* window;
  GtkWidget* wrap_widget(GtkWidget* widget, bool detach_on_destroy = true);
  CXXPointer<Delegator::Connection> on_show_handler;
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

  void on_show(WebKitWebView* widget);
  GtkWidget* create(GimpImageWindow* window);
};

GtkWidget*
GimpImageWindowWebviewPrivate::
wrap_widget (GtkWidget* widget, bool detach_on_destroy) {
  if (gtk_widget_get_parent(widget) != NULL)
    return NULL;
  GtkWidget* eventbox = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (eventbox),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(eventbox), widget);
  if (detach_on_destroy) {
    decorator(eventbox, new PluginDetacher(widget))
      .connect("destroy", &PluginDetacher::on_destroy);
  }
  if (GIMP_IS_DOCKABLE(widget))
    decorator(eventbox, new DockableWrapper(GIMP_DOCKABLE(widget)))
      .connect("destroy", &DockableWrapper::on_destroy);
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
      result = wrap_widget(result);
      
    } else if (strncmp(uri, "/dock", strlen("/dock")) == 0) {
      gchar* dock_name = uri + strlen("/dock");
      if (strcmp(dock_name,"/left") == 0) {
        g_print("WEBVIEW:left_docks=%lx\n", (gulong)priv->left_docks);
        result = priv->left_docks;
        result = wrap_widget(result);

      } else if (strcmp(dock_name, "/right") == 0) {
        g_print("WEBVIEW:right_docks=%lx\n", (gulong)priv->right_docks);
        result = priv->right_docks;
        result = wrap_widget(result);
      }
    } else if (strcmp(uri, "/images") == 0) {
        g_print("WEBVIEW:images=%lx\n", (gulong)priv->notebook);
      result = priv->notebook;
      new ActiveShellConfigurator(window, result);
      result = wrap_widget(result);
      
    } else if (strcmp(uri, "/menubar") == 0) {
        g_print("WEBVIEW:menubar=%lx\n", (gulong)priv->menubar);
      result = priv->menubar;
      result = wrap_widget(result);
      
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
      if (GIMP_IS_DOCKABLE(result)) {
        gimp_dockable_set_context(GIMP_DOCKABLE(result), image_window_get_context(window));
      }
      gtk_widget_show(result);
      result = wrap_widget(result, false);
      g_print("WEBVIEW:dialog[%s]%lx\n", dialog_name.ptr(), (gulong)result);
    }
  }
  return result;
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

void
GimpImageWindowWebviewPrivate::
on_show(WebKitWebView* view) {
  on_show_handler->disconnect();

  // Get file name from config
  StringHolder filename = g_strdup("/tmp/template.html");
  StringHolder file_uri = g_strdup_printf("file://%s", filename.ptr());
  _G(view)[webkit_web_view_load_uri](file_uri);
}

GtkWidget* 
GimpImageWindowWebviewPrivate::create(GimpImageWindow* window)
{
  GtkWidget* result = NULL;
  g_return_val_if_fail (GIMP_IS_IMAGE_WINDOW(window), NULL);
  g_print("WEBVIEW::create: window=%lx\n", (ulong)window);
  this->window = window;
  result = webkit_web_view_new ();
  if (result) {
    decorator(result, this)
      .connect("create-plugin-widget", 
              &GimpImageWindowWebviewPrivate::create_plugin_widget)
      .connect("navigation-policy-decision-requested", 
              &GimpImageWindowWebviewPrivate::navigation_policy_decision_requested);
    on_show_handler = 
      delegator(result, this).connecth("realize", &GimpImageWindowWebviewPrivate::on_show);
  }
  
  return result;
}


extern "C" {

GimpContext* 
image_window_get_context(GimpImageWindow* window) {
  GimpImageWindowPrivate* win_private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  return gimp_get_user_context(win_private->gimp);
};
  
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
