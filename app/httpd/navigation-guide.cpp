/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * navigation-guide
 * Copyright (C) 2017 seagetch <sigetch@gmail.com>
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

#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-bridge.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"

#include <gegl.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "libgimpcolor/gimpcolor.h" // GIMP_TYPE_RGB
#include "libgimpbase/gimpbase.h"   // GIMP_TYPE_PARASITE

#include "core/core-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimplayer.h"
#include "core/gimpdrawable.h"
#include "core/gimpitem.h"
#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpcontext.h"

#include "widgets/widgets-types.h"
#include "display/display-types.h"
#include "display/gimpimagewindow.h"
#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
}
#include "core/gimpfilterlayer.h"
#include "core/gimpclonelayer.h"


#include "gimp-intl.h"

#include "navigation-guide.h"
//#include "pdb/pdb-cxx-utils.hpp"

template<typename F, typename Ret, typename... Args>
static Ret callback(Args... args, gpointer p) {
  F* f = (F*)p;
  return (*f)(args...);
}

template<typename F>
inline void gimp_container_foreach_(GimpContainer* container, F f) {
  gimp_container_foreach(container, GFunc(callback<F, void, GimpItem*>), &f);
}

namespace GLib {
#if 0
template<> inline GType g_type<GimpLayerModeEffects>() { return GIMP_TYPE_LAYER_MODE_EFFECTS; }
template<> inline GType g_type<GimpImageBaseType>() { return GIMP_TYPE_IMAGE_BASE_TYPE; }
};

enum EMethodId { none = 0, info, data, preview };
struct MethodMap {
  const gchar* method_name;
  EMethodId method_id;
};
#endif
};

class JsonContextAccessor {
public:
  RESTResource*  resource;
  SoupMessage*   message;

  GLib::CString  message_text;
  GLib::CString  webhook_uri;
  GLib::CString  message_id;

  GimpDrawable*  drawable;
  GimpImage*     image;
  Gimp*          gimp;
  GimpPDB*       pdb;
  GimpContext*   context;
  GimpDisplay*    display;
  GimpItem*      item;

  bool configure_args(Gimp* gimp, JSON::INode ctx_json, JSON::INode args_json) {
    auto igimp     = GLib::ref( this->gimp = gimp );
    auto icontext  = GLib::ref( context    = igimp [gimp_get_user_context]() );
    auto iimage    = GLib::ref( image      = icontext [gimp_context_get_image]() );
    auto idrawable = GLib::ref( drawable   = GIMP_DRAWABLE(iimage [gimp_image_get_active_layer]()) );
    auto ipdb      = GLib::ref( pdb        = gimp->pdb );
    auto idisplay  = GLib::ref( display    = GIMP_DISPLAY(icontext [gimp_context_get_display]()) );
    auto iitem     = GLib::ref( item       = GIMP_ITEM(drawable) );

    auto imessage  = GLib::ref( message );

    // Read variables from context information.
    if (ctx_json.is_object()) {
      g_print("context is object\n");
      for ( auto key: GLib::ref<const gchar*>(ctx_json.keys()) ) {
        g_print("Key=%s\n", key);
        try {
          JSON::INode id_node = ctx_json[key];

          if (strcmp(key, "image") == 0) {
            gint id   = id_node;
            iimage    = GLib::ref( image = gimp_image_get_by_ID (gimp, id) );

          } else if (strcmp(key, "drawable") == 0) {
            gint id   = id_node;
            idrawable = GLib::ref( drawable = GIMP_DRAWABLE(gimp_item_get_by_ID (gimp, id)) );

          } else if (strcmp(key, "item") == 0) {
            gint id   = id_node;
            iitem     = GLib::ref( item = gimp_item_get_by_ID (gimp, id) );

          } else if (strcmp(key, "display") == 0 && gimp->gui.display_get_by_id) {
            gint id   = id_node;
            idisplay  = GLib::ref( display = GIMP_DISPLAY(gimp->gui.display_get_by_id (gimp, id)) );

          }

        } catch(JSON::INode::InvalidType e) {
          g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
        }
      }
    }

    if (args_json.is_object()) {
      g_print("arguments is object\n");
      try {
        this->message_text = g_strdup (args_json["message"]);
        this->webhook_uri  = g_strdup (args_json["webhook_uri"]);
        this->message_id   = g_strdup (args_json["message_id"]);
      } catch(JSON::INode::InvalidType e) {
        g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
      }
    }
  }


  JsonNode* get_context_json() {
    // Read variables from context information.
    return JSON::build_object([this](auto it){
      gint id;
      it["image"]    = gimp_image_get_ID(image);
      it["drawable"] = gimp_item_get_ID(GIMP_ITEM(drawable));
      it["item"]     = gimp_item_get_ID(item);
    });
  }
};

///////////////////////////////////////////////////////////////////////
// Navigation REST interface

struct NavigationGuideContext {
  JsonContextAccessor* arg_conf;
};

void on_guide_finished (NavigationGuideContext* navi_context)
{
  if (navi_context->arg_conf && navi_context->arg_conf->webhook_uri) {
    JSON::INode new_root = JSON::build_object([&](auto it) {
      it["context"] = navi_context->arg_conf->get_context_json();
    });
    GLib::CString result_text = json_to_string(new_root, FALSE);
    g_print("uri=%s, id=%s, result=%s\n", navi_context->arg_conf->webhook_uri.ptr(), navi_context->arg_conf->message_id.ptr(), result_text.ptr());

    SoupSession* session = soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_SNIFFER, NULL);
    SoupLogger* logger = soup_logger_new (SOUP_LOGGER_LOG_MINIMAL, -1);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(logger));
    g_object_unref(logger);

    SoupMessage* hook_msg;
    hook_msg = soup_message_new("POST", navi_context->arg_conf->webhook_uri.ptr());
    auto imessage = GLib::ref(hook_msg);
    imessage [soup_message_set_request] ("application/json; charset=utf-8", SOUP_MEMORY_COPY,
                                          (const gchar*)result_text, strlen(result_text));
//    soup_message_set_request(hook_msg, "application/x-www-form-urlencoded", SOUP_MEMORY_COPY, data, strlen(data));
    GError* error = NULL;
    gint status = soup_session_send_message (session, hook_msg);
    g_print("status_code = %d\n", status);
    g_object_unref(hook_msg);
    delete navi_context->arg_conf;
  }
}

class NavigationGuide : public RESTResource {

public:
  NavigationGuide(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) :
    RESTResource(gimp, matched, msg, context) { }
  virtual void get();
  virtual void put();
  virtual void post();
  virtual void del();
};


RESTResource*
NavigationGuideFactory::create(Gimp* gimp,
                             RESTD::Router::Matched* matched,
                             SoupMessage* msg,
                             SoupClientContext* context)
{
  return new NavigationGuide(gimp, matched, msg, context);
}


void NavigationGuide::get()
{
  const gchar*            path       = matched->data()["**"];

}


void NavigationGuide::put()
{
  const gchar*            path       = matched->data()["**"];
}


void NavigationGuide::post()
{
  JSON::INode json = req_body_json();
  if (!json.is_object())
    return;

  try {
    g_print("try to get context/arguments\n");
    auto ctx_node  = json["context"];
    auto args_node = json["arguments"];
    GLib::IObject<GimpImage> image;

    auto arg_conf = new JsonContextAccessor;
    arg_conf->message  = message;
    arg_conf->resource = this;

    g_print("do parse message\n");
    arg_conf->configure_args(gimp, ctx_node, args_node);

    g_print("done parse message\n");
    NavigationGuideContext* guide_context = g_new0(NavigationGuideContext,1);
    guide_context->arg_conf    = arg_conf;

    if (arg_conf->display) {
      GimpDisplayShell *shell = gimp_display_get_shell (arg_conf->display);
      if (shell) {
        GimpImageWindow *window = gimp_display_shell_get_window (shell);
        if (window)
          gimp_image_window_activate_navigation_bar (window, guide_context->arg_conf->message_text, NULL, G_CALLBACK(on_guide_finished), guide_context);
      }
    }

  } catch(JSON::INode::InvalidType e) {
  } catch(JSON::INode::InvalidIndex e) {
  }
}


void NavigationGuide::del()
{

}
