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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpbase/gimpbase.h"
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

#include <cmath>
#include <vector>
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
static GimpContext* image_window_get_context(GimpImageWindow* window);
}

/////////////////////////////////////////////////////////////////////////////
/// Class Route
///  Manage mapping from URI to handler delegators.
template<typename Ret, typename... Args>
class Route {
public:
  struct Matched {
    char** path;
    GHashTableHolder<gchar*, gchar*> data;
    Matched() : path(NULL), data(g_hash_table_new(g_str_hash, g_str_equal)) {}
    Matched(const Matched& src) : path(g_strdupv((gchar**)src.path)), data(g_hash_table_ref((GHashTable*)src.data.ptr())) {}
    ~Matched() {
      if (path)
        g_strfreev(path);
      path = NULL;
    }
  };
  typedef bool rule_handler(Matched*);
  typedef Delegator::Delegator<Ret, Matched*, Args...> rule_delegator;

protected:
  struct Rule {
    struct TokenRule { virtual bool test(const gchar* token, Matched& result) = 0; };
    struct Match: TokenRule {
      StringHolder token;
      Match(const gchar* name) : token(g_strdup(name)) {}
      Match(const Match& src) : token(g_strdup(src.token)) {}
      virtual bool test(const gchar* tested_token, Matched& result) {
//        g_print("  - match:%s<->%s\n", token.ptr(), tested_token);
        return strcmp(token, tested_token) == 0;
      }
    };
    struct Name: TokenRule {
      StringHolder name;
      Name(const gchar* n) : name(g_strdup(n)) {}
      Name(const Name& src) : name(g_strdup(src.name)) {}
      virtual bool test(const gchar* tested_token, Matched& result) {
//        g_print("  - name:[%s]=%s\n", name.ptr(), tested_token);
        g_hash_table_insert(result.data, name, g_strdup(tested_token));
        return true;
      }
    };
    struct Select: TokenRule {
      StringHolder name;
      gchar** candidates;
      Select(const gchar* n, gchar** c) : name(g_strdup(n)), candidates(g_strdupv(c)) {}
      Select(const Select& src) : name(g_strdup(src.name)), candidates(g_strdupv(src.candidates)) {}
      ~Select() {
        if (candidates)
          g_strfreev(candidates);
      }
      virtual bool test(const gchar* tested_token, Matched& result) {
        for (gchar** next_token = candidates; *next_token; next_token ++) {
//          g_print("  - select:[%s](%s<=>%s)\n", name.ptr(), *next_token, tested_token);
          if (strcmp(*next_token, tested_token) == 0) {
            g_hash_table_insert(result.data, name, g_strdup(tested_token));
            return true;
          }
        }
        return false;
      }
    };
    std::vector<TokenRule*> rules;
    rule_delegator* handler;
    
    Rule(const gchar* path, rule_delegator* h) {
      handler = h;
      g_print("Rule::constructor parse %s\n", path);
      parse_rule(path);
    }
    
    void parse_rule(const gchar* path) {
      ScopedPointer<gchar*, void (gchar**), g_strfreev> rule_path(g_strsplit(path, "/", 256));
      for (gchar** token = rule_path.ptr(); *token; token ++) {
        if (!*token || strlen(*token) == 0)
          continue;
        g_print("Rule::constructor parse: transform %s\n", *token);
        switch ((*token)[0]) {
        case '<': {
            gchar* word_term = strstr(&((*token)[1]),">");
            if (word_term) {
              StringHolder varname = strndup(&((*token)[1]), word_term - &((*token)[1]));

              gchar* type_term = strstr(varname.ptr(), ":");
              if (type_term) { // Have type declaration or selection
                StringHolder type = strndup(varname.ptr(), type_term - varname.ptr());
                StringHolder subname = strdup(type_term + 1);
                ScopedPointer<gchar*, void (gchar**), g_strfreev> candidates(g_strsplit(type.ptr(), "|", 256));
                g_print("register Select(%s)\n", subname.ptr());
                rules.push_back(new Select(subname.ptr(), candidates.ptr()));
              } else { // Have no type declaration, assume string type.
                g_print("register Name(%s)\n", varname.ptr());
                rules.push_back(new Name(varname.ptr()));
              }
            }
            break;
          }
        default: {
            rules.push_back(new Match(*token));
            g_print("register Match(%s)\n", *token);
            break;
          }
        }
      };
    };
    
    ~Rule() {
      for (auto i = rules.begin(); i != rules.end(); i ++) {
        delete *i;
      }
      rules.clear();
      if (handler)
        delete handler;
      handler = NULL;
    }
    
    Matched test(const gchar** uri, const gchar* data) {
      Matched result;
      result.path = NULL;
      if (rules.size() == 0)
        return result;
      
      gchar** tested_token = (gchar**)uri;
      for (auto i = rules.begin(); i != rules.end(); i ++) {
        const gchar* token = *tested_token;
        while (token && strlen(token) == 0)
          token = *(++tested_token);
        if (!token) { // Tested URI is short.
          return result;
        }
        if (!(*i)->test(token, result))
          return result;
        tested_token ++;
      }
      if (*tested_token)
        return result;
      result.path = g_strdupv((gchar**)uri);
      return result;
    }
    
  };
  std::vector<Route::Rule*> rules;
  
public:
  Route() {}
  ~Route() {
    for (auto i = rules.begin(); i != rules.end(); i ++) {
      delete *i;
    }
    rules.clear();
  }
  int route(const gchar* route, rule_delegator* handler) {
    rules.push_back(new Rule(route, handler));
    return rules.size() - 1;
  };
  Ret dispatch(const gchar* uri, const gchar* data, Ret default_value, Args... args);
};

template<typename Ret, typename... Args>
Ret Route<Ret, Args...>::dispatch(const gchar* uri, const gchar* data, Ret default_value, Args... args) {
  ScopedPointer<gchar*, void (gchar**), g_strfreev> tested_path = g_strsplit(uri, "/", 256);
  for (auto i = rules.begin(); i != rules.end(); i ++) {
    Matched route = (*i)->test((const gchar**)tested_path.ptr(), data);
    if (route.path) {
      return (*i)->handler->emit(&route, args...);
    }
  }
  return default_value;
}

#if 0
template<typename... Args>
void Route<void, Args...>::dispatch(const gchar* uri, const gchar* data, Args... args) {
  ScopedPointer<gchar*, void (gchar**), g_strfreev> tested_path = g_strsplit(uri, "/", 256);
  for (auto i = rules.begin(); i != rules.end(); i ++) {
    Matched route = (*i)->test((const gchar**)tested_path.ptr(), data);
    if (route.path) {
      (*i)->handler->emit(&route, args...);
      break;
    }
  }
}
#endif
/////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////
class GimpImageWindowWebviewPrivate {
private:
  GimpImageWindow* window;
  GtkWidget* wrap_widget(GtkWidget* widget, bool detach_on_destroy = true);
  CXXPointer<Delegator::Connection> on_show_handler;
  
  typedef Route<bool,
                WebKitWebFrame*,
                WebKitNetworkRequest*,
                WebKitWebNavigationAction*,
                WebKitWebPolicyDecision*> URIRoute;
  typedef Route<GtkWidget*, GtkWidget*> PluginRoute;
  URIRoute uri_mapper;
  PluginRoute plugin_mapper;
public:
  GimpImageWindowWebviewPrivate();
  void setup_plugin_mapper();
  void setup_uri_mapper();
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

	template<typename F> 
	static Delegator::Delegator<GtkWidget*, PluginRoute::Matched*, GtkWidget*>*
	plugin_builder(F f) {
		std::function<GtkWidget* (PluginRoute::Matched*, GtkWidget*)> func = f;
		return Delegator::delegator(func);
	}

	template<typename F> 
	static Delegator::Delegator<bool, 
	                            URIRoute::Matched*,
														  WebKitWebFrame*,
														  WebKitNetworkRequest*,
														  WebKitWebNavigationAction*,
														  WebKitWebPolicyDecision*>*
	uri_func(F f) {
		std::function<bool (URIRoute::Matched*, WebKitWebFrame*, WebKitNetworkRequest*, WebKitWebNavigationAction*, WebKitWebPolicyDecision*)> func = f;
		return Delegator::delegator(func);
	}

};

GimpImageWindowWebviewPrivate::GimpImageWindowWebviewPrivate() : window(NULL) {
  setup_plugin_mapper();
  setup_uri_mapper();
};

void 
GimpImageWindowWebviewPrivate::setup_plugin_mapper() {
  typedef PluginRoute::Matched Matched;

  plugin_mapper.route("/toolbar", plugin_builder([this](Matched* m, GtkWidget* view) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
    g_print("WEBVIEW:toolbar=%lx\n", (gulong)priv->toolbar);
    return wrap_widget(priv->toolbar);
  }));

  plugin_mapper.route("/dock/<side>", plugin_builder([this](Matched* m, GtkWidget* view) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
	  const gchar* dock_name       = m->data["side"];
	  GtkWidget* result            = NULL;

	  if (strcmp(dock_name,"left") == 0) {
	    g_print("WEBVIEW:left_docks=%lx\n", (gulong)priv->left_docks);
	    result = priv->left_docks;

	  } else if (strcmp(dock_name, "right") == 0) {
	    g_print("WEBVIEW:right_docks=%lx\n", (gulong)priv->right_docks);
	    result = priv->right_docks;
	  }

    return wrap_widget(result);
	}));

  plugin_mapper.route("/images", plugin_builder([this](Matched* m, GtkWidget* view) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
	  GtkWidget* result            = NULL;

	  g_print("WEBVIEW:images=%lx\n", (gulong)priv->notebook);
	  result = priv->notebook;
	  new ActiveShellConfigurator(window, result);
	  return wrap_widget(result);
	}));

  plugin_mapper.route("/menubar", plugin_builder([this](Matched* m, GtkWidget* view) {
    GimpImageWindowPrivate* priv = GIMP_IMAGE_WINDOW_GET_PRIVATE(window);
    return wrap_widget(priv->menubar);
  }));

  plugin_mapper.route("/dialog/<dialog_name>", plugin_builder([this](Matched* m, GtkWidget* view) {
    const gchar* key = "dialog_name";
	  const gchar* dialog_name  = (const gchar*)g_hash_table_lookup(m->data, key);
	  GtkWidget* result         = NULL;
	  GimpUIManager* ui_manager = gimp_image_window_get_ui_manager(window);
	  result = gimp_dialog_factory_dialog_new (gimp_dialog_factory_get_singleton (),
	                                           gtk_widget_get_screen (GTK_WIDGET(view)),
	                                           ui_manager,
	                                           dialog_name, -1, TRUE);
	  if (GIMP_IS_DOCKABLE(result)) {
	    gimp_dockable_set_context(GIMP_DOCKABLE(result), image_window_get_context(window));
	  }
	  gtk_widget_show(result);
	  g_print("WEBVIEW:dialog[%s]%lx\n", dialog_name, (gulong)result);
	  return wrap_widget(result, false);
	}));

};


void 
GimpImageWindowWebviewPrivate::setup_uri_mapper() {
  typedef URIRoute::Matched Matched;
  typedef WebKitWebFrame Frame;
  typedef WebKitNetworkRequest Request;
  typedef WebKitWebNavigationAction Action;
  typedef WebKitWebPolicyDecision Decision;

  uri_mapper.route("/actions/<group>/<cmd>", 
    uri_func([this](Matched* m, Frame* f, Request* r, Action* a, Decision* d)->bool {
		  const gchar* group = m->data["group"];
		  const gchar* cmd   = m->data["cmd"];
      g_print("WEBVIEW:action[%s]->%s  --->", group, cmd);
			GimpUIManager* ui_manager = gimp_image_window_get_ui_manager(window);
			gboolean result = gimp_ui_manager_activate_action(ui_manager, group, cmd);
			if (result)
			  g_print("Found\n");
			else
			  g_print("Not found\n");
			webkit_web_policy_decision_ignore(d);
		  return true; 
		}));

  uri_mapper.route("/actions",
    uri_func([this](Matched* m, Frame* f, Request* r, Action* a, Decision* d)->bool {
			GimpUIManager* ui_manager = gimp_image_window_get_ui_manager(window);
			GList* groups             = gtk_ui_manager_get_action_groups (GTK_UI_MANAGER(ui_manager));
			gint size                 = g_list_length(groups);
			gchar** names             = g_new(gchar*, size + 1);
			gchar** current_name      = names;

      for (GList* list = groups; list; list = g_list_next (list)){
        auto group = GIMP_ACTION_GROUP(list->data);
        *current_name = g_strdup(gtk_action_group_get_name(GTK_ACTION_GROUP(group)));
        current_name ++;
      }

      *current_name = NULL;
      StringHolder strjoined = g_strjoinv("\",\"", names);
      StringHolder strformatted = g_strdup_printf("[\"%s\"]", strjoined.ptr());
      g_print("group=%s\n", strformatted.ptr());

      webkit_web_frame_load_string(f, strformatted.ptr(), "text/json", "utf-8", "");

      g_strfreev(names);
			webkit_web_policy_decision_ignore(d);
		  return true; 
    }));

  uri_mapper.route("/actions/<group>",
    uri_func([this](Matched* m, Frame* f, Request* r, Action* a, Decision* d)->bool {
      const gchar* group_name   = m->data["group"];
			GimpUIManager* ui_manager = gimp_image_window_get_ui_manager(window);
      GtkActionGroup* group     = GTK_ACTION_GROUP(gimp_ui_manager_get_action_group(ui_manager, group_name));
      GList* actions            = gtk_action_group_list_actions (group);
			gint size                 = g_list_length(actions);
			gchar** names             = g_new(gchar*, size + 1);
			gchar** current_name      = names;
      for (GList* list = actions; list; list = g_list_next (list)){
        auto action    = GTK_ACTION(list->data);
        *current_name  = g_strdup(gtk_action_get_name(action));
        current_name ++;
      }
      *current_name = NULL;
      StringHolder strjoined = g_strjoinv("\",\"", names);
      StringHolder strformatted = g_strdup_printf("[\"%s\"]", strjoined.ptr());
      g_print("action=%s\n", strformatted.ptr());
      GWrapper<WebKitNetworkRequest> request = r;
      StringHolder uri = g_strdup((const gchar*)request["uri"]);
      webkit_web_frame_load_string(f, strformatted.ptr(), "text/json", "utf-8", "");
      g_strfreev(names);
			webkit_web_policy_decision_ignore(d);
		  return true; 
    }));

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
    if (strncmp(uri, "file://", strlen("file://")) == 0) {
      uri += strlen("file://");      
      return plugin_mapper.dispatch(uri, NULL, NULL, GTK_WIDGET(view));
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
  StringHolder uri = g_strdup((const gchar*)request["uri"]);

  g_print("WEBVIEW::navigation_policy_decision_requested:%s\n", uri.ptr());
  if (strncmp(uri, "file://", strlen("file://")) == 0) {
    const gchar* path = uri.ptr() + strlen("file://");
    bool result = uri_mapper.dispatch(path, NULL, false, frame, req, action, decision);
    return result;
  }
  return FALSE;
}

void
GimpImageWindowWebviewPrivate::
on_show(WebKitWebView* _view) {
  on_show_handler->disconnect();

  // Get file name from config
  GimpImageWindowPrivate* win_private = GIMP_IMAGE_WINDOW_GET_PRIVATE (window);
  auto view = _G(_view);
  auto config = _G(win_private->gimp->config);

  StringHolder path_origin          = g_strdup(config["webview-html-path"]);
  StringHolder writable_path_origin = g_strdup(config["webview-html-path-writable"]);
  StringHolder writable_path        = gimp_config_path_expand (writable_path_origin, TRUE, NULL);
  StringHolder readable_path        = gimp_config_path_expand (path_origin, TRUE, NULL);

  auto try_load = [&](gchar* path) {
    GListHolder writable_list = path?
      gimp_path_parse (path, 256, TRUE, NULL) : NULL;

    for (GList* item = writable_list.ptr(); item; item = g_list_next(item)) {
      gchar* path = (gchar*)item->data;
      StringHolder filename = g_strdup_printf("%s/index.html", path);
      if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
        g_print("Read html from %s\n", filename.ptr());
        StringHolder file_uri = g_strdup_printf("file://%s", filename.ptr());
        view[webkit_web_view_load_uri](file_uri);
        return true;
      }
    }
    return false;
  };

  try_load(writable_path.ptr()) ||
  try_load(readable_path.ptr()) ||
  [&]{ view[webkit_web_view_load_uri](""); return true; }();
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
