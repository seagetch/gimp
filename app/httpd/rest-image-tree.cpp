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
#include "config.h"

#include <gegl.h>
#include <gio/gio.h>

#include "libgimpcolor/gimpcolor.h" // GIMP_TYPE_RGB
#include "libgimpbase/gimpbase.h"   // GIMP_TYPE_PARASITE

#include "core/core-types.h"
#include "pdb/pdb-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
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

#include "gimp-intl.h"

#include "rest-image-tree.h"
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
template<> inline GType g_type<GimpLayerModeEffects>() { return GIMP_TYPE_LAYER_MODE_EFFECTS; }
template<> inline GType g_type<GimpImageBaseType>() { return GIMP_TYPE_IMAGE_BASE_TYPE; }
};

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
// Image REST interface

class JsonConverter {
  RESTResource* resource;
  Gimp* gimp;
  SoupMessage* message;
public:
  JsonConverter(RESTResource* r, Gimp* g, SoupMessage* msg) : resource(r), gimp(g), message(msg) { };

  GimpImage* to_image(JSON::INode json) {
    if (json.is_object()) {

      try {
        gint x1 = json["boundary"][(guint)0];
        gint y1 = json["boundary"][(guint)1];
        gint x2 = json["boundary"][(guint)2];
        gint y2 = json["boundary"][(guint)3];
        gint w  = x2 - x1;
        gint h  = y2 - y1;
        bool         alpha   = json.has("alpha")? json["alpha"]: true;
        GLib::Enum<GimpImageBaseType> enums;
        GimpImageBaseType image_type;
        try {
          image_type = json.has("color-mode")? enums[json["color-mode"]] : GIMP_RGB;
        } catch (decltype(enums)::IdNotFound e) {
          GLib::CString result_text;
          resource->make_error_response(400, "color-mode \"%s\" is invalid.", (const gchar*)e.identifier);
          return NULL;
        }

        GLib::IObject<GimpImage> image = NULL;
        image = gimp_image_new (gimp, w, h, image_type);

        return image;

      } catch (JSON::INode::InvalidType e) {
        return NULL;
      } catch (JSON::INode::InvalidIndex e) {
        return NULL;
      }

    } else
      return NULL;
  }


  GimpLayer* to_layer(GLib::IObject<GimpItem> item, JSON::INode json)
  {
    try {
      const gchar* name = json["name"];
      const gchar* type = json["type"];
      gint x1 = json["boundary"][(guint)0];
      gint y1 = json["boundary"][(guint)1];
      gint x2 = json["boundary"][(guint)2];
      gint y2 = json["boundary"][(guint)3];
      gint w  = x2 - x1;
      gint h  = y2 - y1;
      GLib::Enum<GimpLayerModeEffects> enum_modes;
      GimpLayerModeEffects mode;
      try {
        mode = json.has("mode")? enum_modes[json["mode"]] : GIMP_NORMAL_MODE;
      } catch(decltype(enum_modes)::IdNotFound e) {
        return NULL;
      }
      double       opacity = json.has("opacity")? json["opacity"]: 1.0;
      bool         visible = json.has("visible")? json["visible"] : true;
      bool         alpha   = json.has("alpha")? json["alpha"]: true;

      GimpLayer* layer = NULL;
      GLib::IObject<GimpImage> image = NULL;

      g_print("get image\n");
      if (GIMP_IS_IMAGE(item.ptr()))
        image = GIMP_IMAGE(item.ptr());
      else if (GIMP_IS_LAYER(item.ptr()))
        image = item [gimp_item_get_image] ();
      g_print("image=%s", image [gimp_image_get_display_name] ());

      g_print("create layer\n");
      GimpImageType image_type = image [gimp_image_base_type_with_alpha] ();
      layer = image [gimp_layer_new] (w, h, image_type, NULL, opacity, mode);

      g_print("return layer\n");
      return layer;

    } catch (JSON::INode::InvalidType e) {
      g_print("Invalid type: assuming type is %d, where actual type is %d.\n", e.specified, e.actual);
      resource->make_error_response(400, "invalid parameters");
      return NULL;
    } catch (JSON::INode::InvalidIndex e) {
      g_print("Boundary out of index\n");
      resource->make_error_response(400, "boundary is invalid.");
      return NULL;
    }
  }

};


class DataConverter {
  RESTResource* resource;
  Gimp* gimp;
  SoupMessage* message;
public:
  DataConverter(RESTResource* r, Gimp* g, SoupMessage* msg) : resource(r), gimp(g), message(msg) { };

  GimpImage* to_image(const gchar* mime_type, const guint8* data, gsize size)
  {
    GError* error = NULL;
    g_print("DataConverter#to_image:: mime=%s, data[%lu]\n", mime_type, size);
    auto loader = GLib::ref( gdk_pixbuf_loader_new_with_mime_type (mime_type, &error) );
    if (error)
      return NULL;

    g_print("read data to loader\n");

    GdkPixbuf* pixbuf = NULL;
    if ( loader [gdk_pixbuf_loader_write] (data, size, &error) ) {
      g_print("load pixbuf\n");
      pixbuf = loader [gdk_pixbuf_loader_get_pixbuf] ();
    }

    if (!pixbuf)
      return NULL;

    return gimp_image_new_from_pixbuf (gimp, pixbuf, _("Imported image"));
  }


  GimpLayer* to_layer(GLib::IObject<GimpItem> item, const gchar* mime_type, const guint8* data, gsize size)
  {
    GError* error = NULL;
    g_print("DataConverter#to_image:: mime=%s, data[%lu]\n", mime_type, size);
    auto loader = GLib::ref( gdk_pixbuf_loader_new_with_mime_type (mime_type, &error) );
    if (error)
      return NULL;

    GdkPixbuf* pixbuf = NULL;
    if ( loader [gdk_pixbuf_loader_write] (data, size, &error) ) {
      pixbuf = loader [gdk_pixbuf_loader_get_pixbuf] ();
    }

    if (!pixbuf)
      return NULL;

    GimpImage* image;

    if (GIMP_IS_IMAGE(item.ptr()))
      image = GIMP_IMAGE(item.ptr());
    else if (GIMP_IS_LAYER(item.ptr()))
      image = item [gimp_item_get_image] ();

    return gimp_layer_new_from_pixbuf (pixbuf, image, GIMP_RGBA_IMAGE, _("Imported layer"), 1.0, GIMP_NORMAL_MODE);
  }

};

class RESTImageTree : public RESTResource {

  GimpImage* data_to_image(const gchar* mime_type, const guint8* data, gsize size);
  GimpLayer* data_to_layer(GLib::IObject<GimpItem> item, const gchar* mime_type, const guint8* data, gsize size);

  void get_info(GLib::IObject<GimpItem> item);
  void get_data(GLib::IObject<GimpItem> item, const gchar* format, gint max_size = 0);
  template<typename Converter, typename... Args> void put_data(GLib::IObject<GimpItem> item, Args... args);
  bool parse_path(const gchar* path, GLib::IObject<GimpItem>& item, EMethodId& method_id);

public:
  RESTImageTree(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) :
    RESTResource(gimp, matched, msg, context) { }
  virtual void get();
  virtual void put();
  virtual void post();
  virtual void del();
};


RESTResource*
RESTImageTreeFactory::create(Gimp* gimp,
                             RESTD::Router::Matched* matched,
                             SoupMessage* msg,
                             SoupClientContext* context)
{
  return new RESTImageTree(gimp, matched, msg, context);
}


void
RESTImageTree::get_info(GLib::IObject<GimpItem> item) {
  auto imessage = GLib::ref(message);

  if (!item) {
    auto container = GLib::ref( gimp->images );

    JSON::Builder builder;
    auto ibuilder = JSON::ref(builder);
    ibuilder = ibuilder.object([&](auto it){
      if (container) {
        gimp_container_foreach_ (container, std::function<void(GimpItem*)>([&](auto i){
          GLib::IObject<GimpItem> item = i;
          GLib::CString id = g_strdup_printf("%d", item [gimp_image_get_ID] ());
          it[(const gchar*)id] = item [gimp_image_get_display_name] ();
        }));
      }
    });
    make_json_response(200, ibuilder.get_root());

  } else {
    if (GIMP_IS_IMAGE(item.ptr())) {
      gint                 w       = item [gimp_image_get_width] ();
      gint                 h       = item [gimp_image_get_height] ();
      const gchar*         type    = "image";

      make_json_response(200,
          JSON::build_object([&](auto it){
            it["name"] = item [gimp_image_get_display_name] ();
            it["type"] = type;
            it["boundary"] = it.array_with_values(0, 0, w, h);
            auto container = item [gimp_image_get_layers] ();
            if (container) {
              it["children"] = it.object([&](auto it){
                gimp_container_foreach_ (container, std::function<void(GimpItem*)>([&](auto i){
                   GLib::IObject<GimpItem> item = i;
                   GLib::CString id = g_strdup_printf("%d", item [gimp_item_get_ID] ());
                   it[(const gchar*)id] = item [gimp_object_get_name] ();
                 }));
              });
            }
          }));

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
      gboolean             alpha   = item [gimp_drawable_has_alpha] ();
      const gchar*         type    = GIMP_IS_GROUP_LAYER(item.ptr())? "group":
                                     GIMP_IS_FILTER_LAYER(item.ptr())? "filter":
                                     GIMP_IS_CLONE_LAYER(item.ptr())? "clone":
                                     "normal";
      GLib::Enum<decltype(mode)> enum_modes;
      auto container = item [gimp_viewable_get_children] ();
      make_json_response(200,
          JSON::build_object([&](auto it){
            it["name"]     = item [gimp_object_get_name] ();
            it["type"]     = type;
            it["mode"]     = enum_modes[mode];//FIXME
            it["opacity"]  = opacity;
            it["boundary"] = it.array_with_values(x, y, x2, y2);
            it["visible"]  = bool(visible);
            it["alpha"]    = bool(alpha);

            if (container) {
              it["children"] = it.array([&](auto it){
                gimp_container_foreach_ (container, std::function<void(GimpItem*)>([&](auto i){
                   GLib::IObject<GimpItem> item = i;
                   it = item [gimp_object_get_name] ();
                 }));
              });
            }

          }));
    }
  }

}


static gboolean
ostream_close(GMemoryOutputStream* stream)
{
  return g_output_stream_close((GOutputStream*)stream, NULL, NULL);
}


void
RESTImageTree::get_data(GLib::IObject<GimpItem> item, const gchar* format, gint max_size)
{
  auto imessage = GLib::ref(message);
  GLib::CString result_text;
  const gchar* item_name = GIMP_IS_IMAGE(item.ptr())?
      item [gimp_image_get_display_name] ():
      item [gimp_object_get_name] ();
  GdkPixbuf* pixbuf = NULL;
  if (!item) {
    make_error_response(404, "\"%s\" is not found.", item_name);
    return;

  } else {

    if (GIMP_IS_IMAGE(item.ptr())) {
      gint w = item [gimp_image_get_width] ();
      gint h = item [gimp_image_get_height] ();
      if (max_size > 0) {
        if (w > h) {
          if (w > max_size)
            h = h * max_size / w;
          w = MIN(w, max_size);
        } else {
          if (h > max_size)
            w = w * max_size / h;
          h = MIN(h, max_size);
        }
      }
      pixbuf = item [gimp_viewable_get_pixbuf] (gimp_get_user_context(gimp), w, h);

    } else if (GIMP_IS_LAYER(item.ptr())) {
      gint w = item [gimp_item_get_width] ();
      gint h = item [gimp_item_get_height] ();
      if (max_size > 0) {
        if (w > h) {
          if (w > max_size)
            h = h * max_size / w;
          w = MIN(w, max_size);
        } else {
          if (h > max_size)
            w = w * max_size / h;
          h = MIN(h, max_size);
        }
      }
      pixbuf = item [gimp_viewable_get_pixbuf] (gimp_get_user_context(gimp), w, h);
    }

    ScopedPointer<GMemoryOutputStream, decltype(ostream_close), ostream_close>
    ostream = (GMemoryOutputStream*)(g_memory_output_stream_new (NULL, 0, g_realloc, g_free));
    GError* error = NULL;

    if (pixbuf) {
      if (gdk_pixbuf_save_to_stream (pixbuf, (GOutputStream*)ostream.ptr(), format, NULL, &error, NULL)) {
        GLib::CString mime_format = g_strdup_printf("image/%s", format);
        soup_message_set_response (message, mime_format, SOUP_MEMORY_COPY,
                                   (const char*)g_memory_output_stream_get_data(ostream),
                                   g_memory_output_stream_get_data_size(ostream));
        imessage.set("status-code", 200);
      } else {
        make_error_response(500, "Failed to convert image data of \"%s\".", item_name);
      }
    } else {
      make_error_response(403, "Cannot convert item(%s) to image data.", item_name);
    }
  }
}


template<typename Converter, typename... Args> void
RESTImageTree::put_data(GLib::IObject<GimpItem> item, Args... args)
{
  auto imessage = GLib::ref(message);
  bool result = false;
  Converter conv(this, gimp, message);

  if (!item) {

    auto image = GLib::ref( conv.to_image(args...) );
    if (!image) {
      result = false;
    } else {
      gimp_create_display (gimp, image, GIMP_UNIT_PIXEL, 1.0);
      result = true;
    }

    auto active_layer = GLib::ref( image [gimp_image_get_active_layer] () );
    make_json_response(
        201,
        JSON::build_object([&](auto it) {
          it["result"] = result;
          it["image"]  = image [gimp_image_get_ID] ();
          if (active_layer) {
            it["layer"]    = active_layer [gimp_item_get_ID] ();
            it["drawable"] = active_layer [gimp_item_get_ID] ();
          }
        }));

  } else {
    auto layer = GLib::ref(conv.to_layer(item, args...));
    GLib::IObject<GimpImage> image;
    GLib::IObject<GimpItem>  parent;
    if (!layer) {
      result = false;
    } else {

      image  = GIMP_IS_IMAGE(item.ptr())? GIMP_IMAGE(item.ptr()) : item [gimp_item_get_image] ();
      parent = GIMP_IS_IMAGE(item.ptr())? NULL: item[gimp_item_get_parent] ();
      gint item_index = GIMP_IS_IMAGE(item.ptr())? 0: GLib::ref(parent [gimp_viewable_get_children] ()) [gimp_container_get_child_index] (item);
      g_print("insert layer=%s to image=%s, with index=%d\n", layer [gimp_object_get_name] (), image [gimp_image_get_display_name] (), item_index);
      image [gimp_image_add_layer] (layer, parent, item_index, FALSE );
      image [gimp_image_flush] ();
      result = true;
    }

    make_json_response(
        201,
        JSON::build_object([&](auto it) {
          it["result"]    = result;
          if (image)
            it["image"]     = image [gimp_image_get_ID] ();
          if (layer) {
            it["layer"]     = layer [gimp_item_get_ID] ();
            it["drawable"]  = layer [gimp_item_get_ID] ();
          }
        }));
  }
}


bool
RESTImageTree::parse_path(const gchar* path, GLib::IObject<GimpItem>& item, EMethodId& method_id)
{
  GLib::StringList        path_list  = g_strsplit(path, "/", -1);

  // Find item in image tree.
  for (int i = 0; path_list && path_list[i]; i ++) {
    const gchar* item_name = path_list[i];
    if (strlen(item_name) == 0)
      continue;

//    g_print("try to find %s\n", item_name);

    if (item_name[0] == '#') {
      const gchar* method_name = &item_name[1];
      g_print("parse method=%s\n", method_name);
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
    gimp_container_foreach_ (container, std::function<void(GimpItem*)>([&](GimpItem* it){
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
      auto imessage = GLib::ref(message);
      make_error_response(404, "\"%s\" is not found.", item_name);
      return false;
    }

    item = next_item;
  }

  return true;
}


void RESTImageTree::get()
{
  const gchar*            path       = matched->data()["**"];
  GLib::IObject<GimpItem> item;
  EMethodId               method_id  = none;

  if (!parse_path(path, item, method_id)) {
    return;
  }

  // Dispatching based on path operation information.
  switch (method_id) {
  case none:
  case info:
    get_info(item);
    break;
  case data:
    get_data(item, "png");
    break;
  case preview:
    get_data(item, "jpeg", 128);
    break;
  default:
    break;
  }

}


void RESTImageTree::put()
{
  const gchar*            path       = matched->data()["**"];
  GLib::IObject<GimpItem> item;
  EMethodId               method_id  = none;

  if (!parse_path(path, item, method_id)) {
    return;
  }

  // Dispatching based on path operation information.
  switch (method_id) {
  case none:
  case info: {
    JSON::INode json = req_body_json();
    put_data<JsonConverter>(item, json);
  }
  break;
  case data: {
    SoupMessageHeaders* headers = req_headers();
    SoupMessageBody*    body    = req_body();

    using PSoupMultipart = ScopedPointer<SoupMultipart, decltype(soup_multipart_free), soup_multipart_free>;
    PSoupMultipart multipart = soup_multipart_new_from_message(headers, body);

    if (multipart) {
      gint length      = soup_multipart_get_length (multipart);

      for (int i = 0; i < length; i ++) {
        SoupMessageHeaders* hdr = NULL;
        SoupBuffer*         bdy = NULL;

        if (! soup_multipart_get_part (multipart, i, &hdr, &bdy))
          continue;

        const gchar* mime_type = soup_message_headers_get_content_type(hdr, NULL);
        if ( strncmp(mime_type, "image/", strlen("image/")) == 0 ) {
          GBytes* bytes        = soup_buffer_get_as_bytes (bdy);
          gsize   size         = 0;
          guint8* content_data = (guint8*)g_bytes_get_data(bytes, &size);
          put_data<DataConverter>(item, mime_type, content_data, size);
          break;
        }
      }

    } else {
      const gchar* mime_type    = soup_message_headers_get_content_type(headers, NULL);
      GBytes*      bytes        = req_body_data();
      gsize        size;
      guint8*      content_data = (guint8*)g_bytes_get_data(bytes, &size);
      put_data<DataConverter>(item, mime_type, content_data, size);

    }
  }
  break;
  case preview:
    break;
  default:
    break;
  }
}


void RESTImageTree::post()
{
}


void RESTImageTree::del()
{

}
