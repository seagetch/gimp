/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPreset
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

#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-bridge.hpp"
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpgrouplayer.h"
#include "core/gimpchannel.h"
#include "core/gimpcontainer.h"

#include "presets-enums.h"

}

#include "core/gimpfilterlayer.h"
#include "core/gimpclonelayer.h"

#include "presets/gimpjsonresource.h"
#include "layer-preset.h"
#include "base/json-cxx-utils.hpp"

using namespace GLib;

////////////////////////////////////////////////////////////////////////////
// LayerPreset application model

static const gchar* TARGET_SOURCE_LABEL = "source";
static const gchar* TARGET_NEW_LABEL    = "new";

static const gchar* LAYER_NORMAL_LABEL = "normal";
static const gchar* LAYER_FILTER_LABEL = "filter";
static const gchar* LAYER_CLONE_LABEL  = "clone";
static const gchar* LAYER_GROUP_LABEL  = "group";
static const gchar* LAYER_ANY_LABEL    = "any";

static const gchar* SOURCE_LABEL    = "source";
static const gchar* SELECTION_LABEL = "selection";

class LayerPresetApplier
{
protected:

  struct Boundary {
    int x1;
    int y1;
    int x2;
    int y2;
  };

  GLib::IObject<GimpContext> context;
  GLib::IObject<GimpJsonResource> preset;


  bool is_source_valid(GimpLayer* source, JSON::INode json) {

    if (!json.is_object())
      return false;

    auto src = ref(source);

    auto _keys = hold( json.keys() );
    auto keys  = ref<const gchar*>(_keys);
    g_print("LayerPresetApplier::is_source_valid\n");

    for (auto key: keys) {

      if (strcmp(key, "type") == 0) {
        CString v = g_strdup(json[key]); // should be one of "normal", "group", "filter", "clone", "any"
        if (strcmp(v, LAYER_NORMAL_LABEL) == 0) {
          bool assert =
              GIMP_IS_LAYER(source) &&
              !GIMP_IS_GROUP_LAYER(source) &&
              !GIMP_IS_FILTER_LAYER(source) &&
              !GIMP_IS_CLONE_LAYER(source);
          if (!assert)
            return false;

        } else if (strcmp(v, LAYER_GROUP_LABEL) == 0) {
          if (!GIMP_IS_GROUP_LAYER(source))
            return false;

        } else if (strcmp(v, LAYER_FILTER_LABEL) == 0) {
          if (!GIMP_IS_FILTER_LAYER(source))
            return false;

        } else if (strcmp(v, LAYER_CLONE_LABEL) == 0) {
          if (!GIMP_IS_CLONE_LAYER(source))
            return false;

        } else if (strcmp(v, LAYER_ANY_LABEL)) {
          // invalid label.
          return false;
        }


      } else if (strcmp(key, "alpha") == 0) {
        bool alpha = json[key];
        if (alpha != src [gimp_drawable_has_alpha] ())
          return false;
      }

    }
    return true;
  }


  Boundary get_boundary(JSON::INode json, GimpLayer* source) {
    auto src = ref(source);
    Boundary result = { 0 };
    if (json.is_array() ) {
      try {
        result.x1 = json[(guint)0];
        result.y1 = json[1];
        result.x2 = json[2];
        result.y2 = json[3];
      } catch(JSON::INode::InvalidIndex e) {
        // FIXME: error check
      }

    } else if (json.is_null() || json.is_value() && json.value_type() == G_TYPE_STRING) {
      // Default should be changed by target ("new", "source")
      const gchar* boundary_text = json.is_null()? "full": json; // source, selection, *full

      if (strcmp(boundary_text, SOURCE_LABEL) == 0) {
        result.x1 = src [gimp_item_get_offset_x] ();
        result.y1 = src [gimp_item_get_offset_y] ();
        result.x2 = result.x1 + src [gimp_item_get_width] ();
        result.y2 = result.y1 + src [gimp_item_get_height] ();

      } else if (strcmp(boundary_text, SELECTION_LABEL) == 0) {
        auto image = ref(context->image);
        auto mask  = image [gimp_image_get_mask] ();
        result.x1  = mask->x1;
        result.y1  = mask->y1;
        result.x2  = mask->x2;
        result.y2  = mask->y2;

      } else if (strcmp(boundary_text, "full") == 0) {
        auto image =ref(context->image);
        result.x1 = 0;
        result.y1 = 0;
        result.x2 = result.x1 + image [gimp_image_get_width] ();
        result.y2 = result.y1 + image [gimp_image_get_height] ();
      }

    }
    return result;
  }


  GimpLayer* get_source(JSON::INode json, GimpLayer* source) {
    GimpLayer* result = NULL;
    if (!json.is_value() || json.value_type() != G_TYPE_STRING)
      return result;

    auto src = ref(source);
    const gchar* source_text = json;

    if (strcmp(source_text, SOURCE_LABEL) == 0) {
      result = source;

    } else if (source_text[0] == '#') {
      //FIXME: how to obtain link? (name may be changed because of name unification.
      result = source;
    }

    return result;
  }


  GimpLayerModeEffects get_mode(JSON::INode json, GimpLayer* source) {
    GimpLayerModeEffects result = (GimpLayerModeEffects)-1;
    if (!json.is_value() || json.value_type() != G_TYPE_STRING)
      return result;

    auto src = ref(source);
    const gchar* source_text = json;

    if (strcmp(source_text, SOURCE_LABEL) == 0) {
      result = src [gimp_layer_get_mode] ();

    } else {
      GEnumClass* enum_class = G_ENUM_CLASS(g_type_class_ref (GIMP_TYPE_LAYER_MODE_EFFECTS));
      GEnumValue* enum_value = g_enum_get_value_by_name (enum_class, source_text);
      if (enum_value)
        result = (GimpLayerModeEffects)enum_value->value;
      g_type_class_unref(enum_class);
    }

    return result;
  }


  double get_opacity(JSON::INode json, GimpLayer* source) {
    double result = -1;
    if (!json.is_value())
      return result;

    auto src = ref(source);

    if (json.value_type() == G_TYPE_STRING) {
      const gchar* opacity_text = json;
      if (strcmp(opacity_text, SOURCE_LABEL) == 0) {
        result = src [gimp_layer_get_opacity] ();

      }
    } else {
      result = json;

    }

    return result;
  }


  bool get_alpha(JSON::INode json, GimpLayer* source) {
    bool result = true;
    auto src = ref(source);

    if (json.is_null()) // default is "source"
      return src [gimp_drawable_has_alpha] ();

    if (!json.is_value())
      return result;

    if (json.value_type() == G_TYPE_STRING) {
      const gchar* opacity_text = json;
      if (strcmp(opacity_text, SOURCE_LABEL) == 0)
        result = src [gimp_drawable_has_alpha] ();
    } else {
      result = json;
    }

    return result;
  }


  GArray* get_args(JSON::INode json, GimpLayer* source) {
    if (!json.is_array())
      return NULL;

    GArray* result = g_array_new(FALSE, TRUE, sizeof(GValue));
    g_array_set_clear_func (result, (GDestroyNotify) g_value_unset);

    auto args = ref<GValue>(result);

    json.each([&](auto arg_json){
      try {
        const gchar* type = arg_json["type"];
        if (strcmp(type, "FLOAT") == 0) {
          float v = (float)(double)arg_json["value"];
          Value value = v;
          args.append(value.ref());

        } else if (strcmp(type, "INT32") == 0) {
          gint v = arg_json["value"];
          Value value = v;
          args.append(value.ref());

        } else if (strcmp(type, "INT8") == 0) {
          guint v = (guint)(int)arg_json["value"];
          Value value = v;
          args.append(value.ref());

        } else if (strcmp(type, "INT8ARRAY") == 0) {
          // FIXME: TBD
          GValue v = G_VALUE_INIT;
          args.append(v);

        } else if (strcmp(type, "RGB") == 0) {
          // FIXME: TBD
          GValue v = G_VALUE_INIT;
          args.append(v);

        } else if (strcmp(type, "STRING") == 0) {
          const gchar* v = arg_json["value"];
          Value value = v;
          args.append(value.ref());

        } else {
          GValue v = G_VALUE_INIT;
          args.append(v);
        }

      } catch (JSON::INode::InvalidType e) {
        GValue v = G_VALUE_INIT;
       args.append(v);
      }

    });
    return args;
  };


  GimpLayer* create_one_layer (JSON::INode node_json, GimpLayer* source) {
    if (!node_json.is_object())
      return NULL;

    GimpLayer*           layer = NULL;

    const gchar* target = node_json["target"]; // *new, source

    const gchar*         name           = node_json["name"];
    GimpLayer*           content_source = get_source  (node_json["source"],   source); // source, *empty
    Boundary             boundary       = get_boundary(node_json["boundary"], source); // source, selection, full, [*,*,*,*]
    GimpLayerModeEffects mode           = get_mode    (node_json["mode"],     source); // source or layer mode
    double               opacity        = get_opacity (node_json["opacity"],  source); // source, double, default(none)
    bool                 alpha          = get_alpha   (node_json["alpha"],    source); // *source, true, false

    auto src = ref(source);

    if (strcmp(target, TARGET_SOURCE_LABEL) == 0) {
      layer = source; // TODO: should be duplicated?

    } else if (strcmp(target, TARGET_NEW_LABEL) == 0){

      const gchar* type = node_json["type"]; // *normal, group, filter, clone

      int width  = boundary.x2 - boundary.x1;
      int height = boundary.y2 - boundary.y1;

      if (!type || strcmp(type, LAYER_NORMAL_LABEL) == 0) { // NORMAL LAYER

        if (content_source) {
          g_print("Duplicate normal layer: %s->%s\n", src[gimp_object_get_name](), name);
          layer = GIMP_LAYER( src [gimp_item_duplicate] (GIMP_TYPE_LAYER) );

        } else {
          g_print("Create normal layer: %s\n", name);
          auto image = ref(context->image);
          GimpImageType image_type = image [gimp_image_base_type_with_alpha] ();
          layer = gimp_layer_new(image,
                                 width, height,
                                 image_type, name, opacity, mode);
          // TODO: alpha
        }

      } else if (strcmp(type, LAYER_GROUP_LABEL) == 0) { //GROUP LAYER

        g_print("Create group layer: %s\n", name);
        layer = gimp_group_layer_new(context->image);
        auto ilayer = ref(layer);

        auto children_json = node_json["children"];

        if (children_json.is_array()) {
          auto _children = hold(this->create_replacement_layer(children_json, source));
          auto children = ref<GimpLayer*>(_children);

          auto container = ref(ilayer [gimp_viewable_get_children] ());

          for ( auto child: children )
            container [gimp_container_add] (GIMP_OBJECT(child));
        }

      } else if (strcmp(type, LAYER_FILTER_LABEL) == 0) { // FILTER LAYER
        g_print("Verifying filter layer parameters: %s\n", name);
        // parse filter layer arguments
        auto filter_json       = node_json["filter"];

        const gchar* proc_name = filter_json["name"];
        auto proc_args = hold(this->get_args(filter_json["args"], source));

        // create filter layer
        if (proc_name && proc_args) {
          g_print("Create filter layer: %s\n", name);
          layer = FilterLayerInterface::new_instance(context->image,
                                                     width, height, name,
                                                     opacity, mode);
          auto filter_layer = FilterLayerInterface::cast(layer);
          filter_layer->set_procedure(proc_name, proc_args);
        }
      } else if (strcmp(type, LAYER_CLONE_LABEL) == 0) { // CLONE LAYER
        g_print("Create clone layer: %s\n", name);
        // create clone layer
        // FIXME: content source may be null at this time.
        layer = CloneLayerInterface::new_instance(context->image,
                                                  content_source, name,
                                                  opacity, mode);
      } else {
        // ERROR
        return NULL;
      }
    }

    if (!layer)
      return NULL;

    auto ilayer = ref(layer);

    if ((gint)mode >= 0)
      ilayer [gimp_layer_set_mode] (mode, TRUE);

    if (boundary.x1 != boundary.x2 && boundary.y1 != boundary.y2) {
      int width  = boundary.x2 - boundary.x1;
      int height = boundary.y2 - boundary.y1;
      int orig_w = ilayer [gimp_item_get_width] ();
      int orig_h = ilayer [gimp_item_get_height] ();

      ilayer [gimp_item_set_offset] (boundary.x1, boundary.y1);

      if (width != orig_w || height != orig_h)
        ilayer [gimp_item_set_size] (width, height);
    }

    if (opacity >= 0 && opacity <= 1.0)
      ilayer [gimp_layer_set_opacity] (opacity, TRUE);

    if (name)
      ilayer [gimp_object_set_name] (name);

    return layer;
  };


  GList* create_replacement_layer(JSON::INode json, GimpLayer* source) {
    g_print("Create replacement layer: \n");

    if (!json.is_array())
      return NULL;

    GList* result = NULL;
    json.each([&](auto node_json) {
      GimpLayer* layer = this->create_one_layer(node_json, source);
      // Append layer to the list.
      if (layer)
        result = g_list_append(result, layer);

    });
    return result;
  };


public:

  LayerPresetApplier(GimpContext* c, GimpJsonResource* p) : context(c), preset(p) { };
  virtual ~LayerPresetApplier();

  GList* create_source_layer(JSON::INode json) {

  }


  void apply_for (GimpLayer* source) {
    auto ipreset = JsonResourceInterface::cast(preset);
    auto json = JSON::ref(ipreset->get_json());
    auto i_source = ref(source); // required to keep instance even if source is removed.

    // Get matching part
    auto source_json = json["source-layer"];

    if (!is_source_valid(source, json)) {
      g_print("Validation failed.\n");
      return;
    }

    // Get application part
    auto replace_json = json["replacement-layer"];

    auto src       = ref(source);
    auto image     = ref(context->image);

    auto container = ref( GIMP_CONTAINER(src [gimp_viewable_get_parent] ()) );
    if (!container)
      container = image [gimp_image_get_layers] ();

    gint item_index = container [gimp_container_get_child_index] (GIMP_OBJECT(source));

    if (item_index < 0) {
      GimpLayer* active_layer = image [gimp_image_get_active_layer] ();
      container = GIMP_CONTAINER(ref(active_layer) [gimp_viewable_get_parent] ());
      if (!container)
        container = image [gimp_image_get_layers] ();
      item_index = container [gimp_container_get_child_index] (GIMP_OBJECT(active_layer));

    } else {
      // Remove source layer.
      container [gimp_container_remove] (GIMP_OBJECT(source));
    }

    // Create layers to be inserted.
    GLib::List _dest_layers = create_replacement_layer(replace_json, source);
    auto dest_layers = ref<GimpLayer*>(_dest_layers);

    // Insert layers.
    for (auto dest_layer : dest_layers) {
      // TODO: insert layer
      container [gimp_container_insert] (GIMP_OBJECT(dest_layer), item_index);
      item_index ++;
    }
  }

  void apply_for_active_layer () {
    auto image = ref(context->image);
    GimpLayer* active_layer = image [gimp_image_get_active_layer] ();
    apply_for (active_layer);

  }

  void apply_for_new_layer () {
    auto ipreset = JsonResourceInterface::cast(preset);
    auto json = JSON::ref(ipreset->get_json());

    // Get matching part
    auto image = ref(context->image);
    GimpLayer* active_layer = image [gimp_image_get_active_layer] ();

    auto source_json = json["source-layer"];

    // FIXME: If "target": "source" is specified in "source-layer", fall in problems.
    GimpLayer* source = create_one_layer(source_json, active_layer);

    apply_for(source);
  }
};


////////////////////////////////////////////////////////////////////////////
// LayerPrestConfig

LayerPresetConfig::LayerPresetConfig() : PresetConfig()
{ 
}


LayerPresetConfig::~LayerPresetConfig() 
{
  GimpDataFactory* factory = get_data_factory();
  if (factory)
    gimp_data_factory_data_free (factory);
}


const gchar*
LayerPresetConfig::get_readonly_path_prop_name ()
{
  return "layer-presets-path";
}


const gchar*
LayerPresetConfig::get_writable_path_prop_name ()
{
  return "layer-presets-path-writable";
}


const gchar*
LayerPresetConfig::get_readonly_default_paths  ()
{
  static const gchar* path = gimp_config_build_data_path ("layer-presets");
  return path;
}


const gchar*
LayerPresetConfig::get_writable_default_paths  ()
{
  static const gchar* path = gimp_config_build_writable_path ("layer-presets");
  return path;
}


////////////////////////////////////////////////////////////////////////////
// LayerPrestConfig::startup code
static LayerPresetConfig* instance = NULL;

PresetConfig*
LayerPresetConfig::config()
{
  if (!instance)
    instance = new LayerPresetConfig();
  return instance;
}
