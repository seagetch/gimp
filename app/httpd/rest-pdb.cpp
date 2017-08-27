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
#include "pdb/gimppdb-query.h"
#include "pdb/gimpprocedure.h"
}

#include "rest-pdb.h"
#include "pdb/pdb-cxx-utils.hpp"


///////////////////////////////////////////////////////////////////////
// Internal PDB caller interface

class JsonArgConfigurator {
public:
  GimpDrawable*  drawable;
  GimpImage*     image;
  Gimp*          gimp;
  GimpPDB*       pdb;
  GimpContext*   context;
  GimpObject*    display;
  GimpItem*      item;

  void configure_args(GimpProcedure* procedure, GValueArray* args, Gimp* gimp, JSON::INode ctx_json, JSON::INode args_json) {
    auto igimp     = GLib::ref( this->gimp = gimp );
    auto icontext  = GLib::ref( context    = igimp [gimp_get_user_context]() );
    auto iimage    = GLib::ref( image      = icontext [gimp_context_get_image]() );
    auto idrawable = GLib::ref( drawable   = GIMP_DRAWABLE(iimage [gimp_image_get_active_layer]()) );
    auto ipdb      = GLib::ref( pdb        = gimp->pdb );
    auto idisplay  = GLib::ref( display    = GIMP_OBJECT(icontext [gimp_context_get_display]()) );
    auto iitem     = GLib::ref( item       = GIMP_ITEM(drawable) );

    // Read variables from context information.
    if (ctx_json.is_object()) {
      for ( auto key: GLib::ref<const gchar*>(ctx_json.keys()) ) {
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
            idisplay  = GLib::ref( display = GIMP_OBJECT(gimp->gui.display_get_by_id (gimp, id)) );

          }

        } catch(JSON::INode::InvalidType e) {
          g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
        }
      }
    }

    // Set context parameter or json_parameter
    for (int i = 0; i < procedure->num_args; i ++) {
      // Overwrite context related arguments.
      GParamSpec* pspec = procedure->args[i];
      try {
        // check by name
        if (strcmp(g_param_spec_get_name(pspec),"run-mode") == 0) {
          g_value_set_int(&args->values[i], GIMP_RUN_NONINTERACTIVE);

        } else {
          // check by type
          GType type = G_PARAM_SPEC_VALUE_TYPE(pspec);
          if ( type == GIMP_TYPE_INT32 ||
               type == G_TYPE_INT ||
               type == G_TYPE_UINT ||
               type ==GIMP_TYPE_INT16 ||
               type ==GIMP_TYPE_INT8) {
            try {
              if (args_json.is_array()) {
                  gint val  = args_json[i];
                  g_value_set_int (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
            }
          } else if (type ==G_TYPE_ENUM) {
          } else if (type ==G_TYPE_BOOLEAN) {
            try {
              if (args_json.is_array()) {
                  bool val  = args_json[i];
                  g_value_set_boolean (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
            }
          } else if (type ==G_TYPE_DOUBLE) {
            try {
              if (args_json.is_array()) {
                  double val  = args_json[i];
                  g_value_set_double (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
            }
          } else if (type ==G_TYPE_STRING) {
            try {
              if (args_json.is_array()) {
                  const gchar* val  = args_json[i];
                  g_value_set_string (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
            }
          } else if (type ==GIMP_TYPE_RGB) {
          } else if (type ==GIMP_TYPE_INT32_ARRAY) {
          } else if (type ==GIMP_TYPE_INT16_ARRAY) {
          } else if (type ==GIMP_TYPE_INT8_ARRAY) {
          } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
          } else if (type ==GIMP_TYPE_STRING_ARRAY) {
          } else if (type ==GIMP_TYPE_COLOR_ARRAY) {
          } else if (type ==GIMP_TYPE_ITEM_ID ||
                     type ==GIMP_TYPE_DISPLAY_ID ||
                     type ==GIMP_TYPE_IMAGE_ID ||
                     type ==GIMP_TYPE_LAYER_ID ||
                     type ==GIMP_TYPE_CHANNEL_ID ||
                     type ==GIMP_TYPE_DRAWABLE_ID ||
                     type ==GIMP_TYPE_SELECTION_ID ||
                     type ==GIMP_TYPE_LAYER_MASK_ID ||
                     type ==GIMP_TYPE_VECTORS_ID) {
            bool error = false;

            try {
              if (args_json.is_array()) {
                  gint id  = args_json[i];
                  g_value_set_int (&args->values[i], id);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
              error = true;
            } catch(JSON::INode::InvalidType e) {
              g_print("%s: Invalid type,expect=%d, where actual=%d\n", pspec->name, e.specified, e.actual);
              error = true;
            }

            if (error) {
              if (type == GIMP_TYPE_IMAGE_ID) {
                if (image) {
                  g_print("set current image=%d\n", gimp_image_get_ID(image));
                  gimp_value_set_image (&args->values[i], image);
                }
              } else if (type == GIMP_TYPE_ITEM_ID) {
                if (item) {
                  g_print("set current item=%d\n", gimp_item_get_ID(item));
                  gimp_value_set_item (&args->values[i], item);
                }
              } else if (type == GIMP_TYPE_DRAWABLE_ID) {
                if (drawable) {
                  g_print("set current drawable=%d\n", gimp_item_get_ID(GIMP_ITEM(drawable)));
                  gimp_value_set_drawable (&args->values[i], drawable);
                }
              } else if (GIMP_TYPE_DISPLAY_ID) {
                if (display) {
                  g_print("set current display\n");
                  gimp_value_set_display (&args->values[i], display);
                }
              }
            }
          } else if (type == GIMP_TYPE_PARASITE) {
          } else if (type == GIMP_TYPE_PDB_STATUS_TYPE) {
          }
        } // else

      } catch(JSON::INode::InvalidType e) {
        g_print("Invalid type,expect=%d, where actual=%d\n", e.specified, e.actual);
      }

    } // for
  }


  JsonNode* get_context_json() {
    // Read variables from context information.
    gint id;
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    json_builder_set_member_name (builder, "image");
    id   = gimp_image_get_ID(image);
    json_builder_add_int_value(builder, id);

    json_builder_set_member_name (builder, "drawable");
    id   = gimp_item_get_ID(GIMP_ITEM(drawable));
    json_builder_add_int_value(builder, id);

    json_builder_set_member_name (builder, "item");
    id   = gimp_item_get_ID(item);
    json_builder_add_int_value(builder, id);

    json_builder_end_object(builder);
    JsonNode* result = json_builder_get_root(builder);
    g_object_unref(G_OBJECT(builder));

    return result;
  }
};

class JsonPDBSyncExecutor : PDBSyncExecutor {
public:
  JsonNode* result_json;

  JsonPDBSyncExecutor() : PDBSyncExecutor(), result_json(NULL) { };
  ~JsonPDBSyncExecutor() {
    if (result_json) {
      json_node_unref (result_json);
      result_json = NULL;
    }
  };

  void execute(GimpProcedure* procedure, Gimp* gimp, GimpContext* context,
               GimpProgress* progress, GValueArray* args, GimpObject* display)
  {
    PDBSyncExecutor::execute(procedure, gimp, context, progress, args, display);

    // Serialize return value
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_array(builder);
    for (int i = 0; i < result->n_values; i ++) {
      GType type = G_VALUE_TYPE(&result->values[i]);
      if ( type == GIMP_TYPE_INT32 ||
           type == G_TYPE_INT ||
           type == G_TYPE_UINT ||
           type ==GIMP_TYPE_INT16 ||
           type ==GIMP_TYPE_INT8) {
        gint val = g_value_get_int (&result->values[i]);
        json_builder_add_int_value(builder, val);

      } else if(type ==G_TYPE_ENUM) {
      } else if(type ==G_TYPE_BOOLEAN) {
        gboolean val = g_value_get_boolean (&result->values[i]);
        json_builder_add_boolean_value(builder, val);

      } else if (type ==G_TYPE_DOUBLE) {
        gdouble val = g_value_get_double (&result->values[i]);
        json_builder_add_double_value(builder, val);

      } else if (type ==G_TYPE_STRING) {
        const gchar* val = g_value_get_string (&result->values[i]);
        json_builder_add_string_value(builder, val);

      } else if (type ==GIMP_TYPE_RGB) {
      } else if (type ==GIMP_TYPE_INT32_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_INT16_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_INT8_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_STRING_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_COLOR_ARRAY) {
        break;
      } else if (type ==GIMP_TYPE_ITEM_ID ||
                 type ==GIMP_TYPE_LAYER_ID ||
                 type ==GIMP_TYPE_CHANNEL_ID ||
                 type ==GIMP_TYPE_DRAWABLE_ID ||
                 type ==GIMP_TYPE_SELECTION_ID ||
                 type ==GIMP_TYPE_LAYER_MASK_ID ||
                 type ==GIMP_TYPE_VECTORS_ID ||
                 type ==GIMP_TYPE_DISPLAY_ID ||
                 type ==GIMP_TYPE_IMAGE_ID) {
        gint id = g_value_get_int(&result->values[i]);
        json_builder_add_int_value(builder, id);
      } else if (type == GIMP_TYPE_PARASITE) {
      } else if (type == GIMP_TYPE_PDB_STATUS_TYPE) {
      }

    }
    json_builder_end_array(builder);
    result_json = json_builder_get_root(builder);
    g_object_unref(G_OBJECT(builder));
/*
    // Flush image.
    auto igimp     = GLib::ref( gimp );
    auto icontext  = GLib::ref( igimp [gimp_get_user_context]() );
    auto iimage    = GLib::ref( icontext [gimp_context_get_image]() );
    gimp_image_flush (iimage);
    */
  }
};

using JsonPDBRunner = ProcedureRunnerImpl<JsonArgConfigurator, JsonPDBSyncExecutor>;

///////////////////////////////////////////////////////////////////////
// PDB REST interface

void RESTPDB::get()
{
  const gchar* proc_name = matched->data()["name"];
  g_print("proc_name=%s\n", proc_name);

  if (!proc_name) {
    // listing proc
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_array(builder);

    auto             pdb      = GLib::ref( gimp->pdb );
    gint             num_procs;
    gchar**          procs = NULL;
    gboolean         query = pdb [gimp_pdb_query] (".*", ".*", ".*", ".*", ".*", ".*", ".*", &num_procs, &procs, NULL);

    for (int i = 0; i < num_procs; i ++) {
      json_builder_add_string_value(builder, procs[i]);
      g_free(procs[i]);
    }
    g_free(procs);
    json_builder_end_array(builder);
    auto result_json = JSON::ref(json_builder_get_root(builder));
    GLib::CString result_text = json_to_string(result_json, FALSE);
    g_print("result=%s\n", (const gchar*)result_text);
    soup_message_set_response (message,
                               "text/json",
                               SOUP_MEMORY_COPY,
                               result_text,
                               strlen(result_text));
    g_object_unref(G_OBJECT(builder));
  } else {

  }
}


void RESTPDB::put()
{

}


void RESTPDB::post()
{
  JSON::INode json = req_body_json();
  if (!json.is_object())
    return;

  try {
    auto ctx_node = json["context"];
    GLib::IObject<GimpImage> image;

    if (ctx_node.is_object()) {

    } else {
      return;
    }

    const gchar* proc_name = matched->data()["name"];
    auto arg_node = json["arguments"];

    GimpProcedure* procedure = NULL;
    if (proc_name) {
      g_print("proc_name=%s\n", proc_name);
      procedure = GLib::ref(gimp->pdb) [gimp_pdb_lookup_procedure] (proc_name);
    }

    if (procedure) {

      JsonPDBRunner runner(procedure, NULL);
      runner.run(gimp, ctx_node, arg_node);

      auto arg_conf = runner.get_arg_configurator();
      auto executor = runner.get_executor();

      JSON::Node new_ctx_json = arg_conf->get_context_json();
      JSON::Node result_json  = executor->result_json;

      JSON::Node new_root     = json_node_new(JSON_NODE_OBJECT);
      json_node_set_object (new_root, json_object_new());
      GLib::CString ctx_text = json_to_string(new_ctx_json, FALSE);

      JsonObject* jobj = json_node_get_object(new_root);
      json_object_set_member (jobj, "context", new_ctx_json);
      json_object_set_member (jobj, "result", result_json);

      GLib::CString root_text = json_to_string(new_root, FALSE);
      g_print("root=%s\n", (const gchar*)root_text);

      soup_message_set_response (message,
                                 "text/json",
                                 SOUP_MEMORY_COPY,
                                 root_text,
                                 strlen(root_text));
    } else {
      g_print("invalid procedure name.\n");
    }

  } catch(JSON::INode::InvalidType e) {
  } catch(JSON::INode::InvalidIndex e) {

  }
}


void RESTPDB::del()
{

}
