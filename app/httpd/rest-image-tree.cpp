/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * rest-pdb
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
#include <gegl.h>

#include "libgimpcolor/gimpcolor.h" // GIMP_TYPE_RGB
#include "libgimpbase/gimpbase.h"   // GIMP_TYPE_PARASITE

#include "core/core-types.h"
#include "pdb/pdb-types.h"

#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpdrawable.h"
#include "core/gimpitem.h"
#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "pdb/gimppdb-query.h"
#include "pdb/gimpprocedure.h"
#include "core/gimpgrouplayer.h"
}
#include "core/gimpfilterlayer.h"
#include "core/gimpclonelayer.h"

#include "rest-image-tree.h"
//#include "pdb/pdb-cxx-utils.hpp"

template<typename F>
struct FunctionBase { };

template<typename Ret, typename... Args>
struct FunctionBase<Ret(Args...)> {
  template<typename F>
  static Ret callback(Args... args, F* f) {
    return (*f)(args...);
  }
};

template<typename Decl, typename F>
struct Function {
  using Callback = decltype(&FunctionBase<Decl>::template callback<F>);
  Callback callback;
  F f;
};

template<typename F, typename Ret, typename... Args>
struct Function<Ret(Args...), F> {
  using Callback = decltype(&FunctionBase<Ret(Args...)>::template callback<F>);
  Callback callback;
  F f;
  Ret operator() (Args... args) {
    (*callback)(args..., &f);
  }
};

template<typename Decl, typename F>
auto function(F f) {
  Function<Decl, F> holder = {
      &FunctionBase<Decl>::template callback<F>,
      f
  };
  return holder;
}

template<typename F>
inline void gimp_container_foreach_(GimpContainer* container, Function<void(GimpItem*), F> f) {
  gimp_container_foreach(container, GFunc(f.callback), &f.f);
}

enum EMethodId { none = 0, info, data, preview };
struct MethodMap {
  const gchar* method_name;
  EMethodId method_id;
};

MethodMap method_map[] = {
    { "info", info },
    { "data", data },
    { "preview", preview }
};

///////////////////////////////////////////////////////////////////////
// PDB REST interface

void RESTImageTree::get()
{
  auto imessage = GLib::ref(message);
  const gchar* path = matched->data()["**"];

  GLib::CString result_text = g_strdup_printf("{'path': '%s.' }", (const gchar*)path);

  GLib::StringList path_list = g_strsplit(path, "/", -1);
  GLib::IObject<GimpItem> item = NULL;

  EMethodId method_id = none;

  for (int i = 0; path_list && path_list[i]; i ++) {
    const gchar* item_name = path_list[i];
    if (strlen(item_name) == 0)
      continue;

    if (item_name[0] == '#') {
      const gchar* method_name = &item_name[1];
      for (int j = 0; j < sizeof(method_map) / sizeof(MethodMap); j ++) {
        MethodMap* map = &method_map[j];
        if (strcmp(map->method_name, method_name) == 0) {
          method_id = map->method_id;
          break;
        }
      }
      break;
    }

    GLib::IObject<GimpContainer> container = NULL;

    if (!item) {
      container = gimp->images;
    } else if (GIMP_IS_IMAGE(item.ptr())){
      container = item [gimp_image_get_layers] ();
    } else {
      container = item [gimp_viewable_get_children] ();
    }

    GimpItem* next_item = NULL;
    gimp_container_foreach_ (container, function<void(GimpItem*)>([&](GimpItem* it){
      auto this_item = GLib::ref(it);
      if (GIMP_IS_IMAGE(it)) {
        gchar* endptr;
        glong id = g_ascii_strtoll (item_name, &endptr, 10);
        if (!id && endptr == item_name) {
          if (strcmp(this_item [gimp_image_get_display_name](), item_name) == 0)
            next_item = this_item;
        } else {
          if (this_item [gimp_image_get_ID]() == id)
            next_item = this_item;
        }
      } else {
        gchar* endptr;
        glong id = g_ascii_strtoll (item_name, &endptr, 10);
        if (!id && endptr == item_name) {
          if (strcmp(this_item [gimp_object_get_name](), item_name) == 0 )
            next_item = this_item;
        } else {
          if (this_item [gimp_item_get_ID]() == id)
            next_item = this_item;
        }
      }
    }));

    if (!next_item) {
      // Not matched to item tree.
      result_text = g_strdup_printf("{'error': '%s is not found.' }", item_name);
      soup_message_set_response (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY,
                                 result_text, strlen(result_text));
      imessage.set("status-code", 404);
      return;
    }

    item = next_item;
  }

  switch (method_id) {
  case none:
  case info:
    if (!item) {
      auto container = GLib::ref( gimp->images );
      JSON::Builder builder;
      auto ibuilder = JSON::ref(builder);
      ibuilder = ibuilder.object([&](auto it){
        if (container) {
          gimp_container_foreach_ (container, function<void(GimpItem*)>([&](auto i){
             GLib::IObject<GimpItem> item = i;
             GLib::CString id = g_strdup_printf("%d", item [gimp_image_get_ID] ());
             it[(const gchar*)id] = item [gimp_image_get_display_name] ();
           }));
        }
      });
      result_text = json_to_string(ibuilder.get_root(), FALSE);

    } else {
      if (GIMP_IS_IMAGE(item.ptr())) {
        gint                 w       = item [gimp_image_get_width] ();
        gint                 h       = item [gimp_image_get_height] ();
        const gchar*         type    = "image";
        JSON::Builder builder;
        auto ibuilder = JSON::ref(builder);
        ibuilder = ibuilder.object([&](auto it){
          it["name"] = item [gimp_image_get_display_name] ();
          it["type"] = type;
          it["boundary"] = it.array([&](auto it){
            it = 0;
            it = 0;
            it = w;
            it = h;
          });
          auto container = item [gimp_image_get_layers] ();
          if (container) {
            it["children"] = it.object([&](auto it){
              gimp_container_foreach_ (container, function<void(GimpItem*)>([&](auto i){
                 GLib::IObject<GimpItem> item = i;
                 GLib::CString id = g_strdup_printf("%d", item [gimp_item_get_ID] ());
                 it[(const gchar*)id] = item [gimp_object_get_name] ();
               }));
            });
          }
        });
        result_text = json_to_string(ibuilder.get_root(), FALSE);

      } else if (GIMP_IS_LAYER(item.ptr())) {
        GimpLayerModeEffects mode    = item [gimp_layer_get_mode] ();
        gdouble              opacity = item [gimp_layer_get_opacity] ();
        gint                 x       = item [gimp_item_get_offset_x] ();
        gint                 y       = item [gimp_item_get_offset_y] ();
        gint                 w       = item [gimp_item_get_width] ();
        gint                 h       = item [gimp_item_get_height] ();
        gint                 x2      = x + w;
        gint                 y2      = y + h;
        gboolean             visible = item [gimp_item_get_visible] ();
        const gchar*         type    = GIMP_IS_GROUP_LAYER(item.ptr())? "group":
                                       GIMP_IS_FILTER_LAYER(item.ptr())? "filter":
                                       GIMP_IS_CLONE_LAYER(item.ptr())? "clone":
                                       "normal";
        JSON::Builder builder;
        auto ibuilder = JSON::ref(builder);
        ibuilder = ibuilder.object([&](auto it){
          it["name"] = item [gimp_object_get_name] ();
          it["type"] = type;
          it["mode"] = mode;//FIXME
          it["opacity"] = opacity;
          it["boundary"] = it.array([&](auto it){
            it = x;
            it = y;
            it = x2;
            it = y2;
          });
          it["visible"] = bool(visible);
          auto container = item [gimp_viewable_get_children] ();
          if (container) {
            it["children"] = it.array([&](auto it){
              gimp_container_foreach_ (container, function<void(GimpItem*)>([&](auto i){
                 GLib::IObject<GimpItem> item = i;
                 it = item [gimp_object_get_name] ();
               }));
            });
          }
        });
        result_text = json_to_string(ibuilder.get_root(), FALSE);
      }

    }
    break;
  case data:
    break;
  case preview:
    break;
  default:
    break;
  }

  soup_message_set_response (message, "application/json; charset=utf-8", SOUP_MEMORY_COPY,
                             result_text, strlen(result_text));
  imessage.set("status-code", 200);
}


void RESTImageTree::put()
{

}


void RESTImageTree::post()
{
}


void RESTImageTree::del()
{

}
