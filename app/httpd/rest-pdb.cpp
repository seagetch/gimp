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
// Swagger interface

class ProcedurePublisher {
public:
  static bool use_named_args(GimpProcedure* procedure) {
    // Check whether argument ID is unique or not.
    bool use_object_based_args = true;
    for (int i = 0; i < procedure->num_args; i ++) {
      GParamSpec* pspec = procedure->args[i];
      for (int j = 0; j < i; j ++) {
        GParamSpec* pspec2 = procedure->args[j];
        if (strcmp(pspec->name, pspec2->name) == 0) {
          use_object_based_args = false;
          break;
        }
      }
    }
    return use_object_based_args;
  }

  static bool use_named_values(GimpProcedure* procedure) {
    // Check whether argument ID is unique or not.
    bool use_object_based_values = true;
    for (int i = 0; i < procedure->num_values; i ++) {
      GParamSpec* pspec = procedure->values[i];
      for (int j = 0; j < i; j ++) {
        GParamSpec* pspec2 = procedure->values[j];
        if (strcmp(pspec->name, pspec2->name) == 0) {
          use_object_based_values = false;
          break;
        }
      }
    }
    return use_object_based_values;
  }

  static gchar* prop_name(GParamSpec* pspec, bool use_named, int i) {
    if (strcmp(pspec->name, "run-mode") == 0)
      return g_strdup(pspec->name);
    return use_named? g_strdup(pspec->name): g_strdup_printf("%d", i);
  }
};


///////////////////////////////////////////////////////////////////////
// Swagger v2.0 declaration

class ProcedurePublisherOAS2 : public ProcedurePublisher {
public:
  JSON::IBuilder& define_param_spec(JSON::IBuilder& it, bool use_named, GParamSpec* pspec, int index) {
    GLib::CString val_name = prop_name(pspec, use_named, index);
    it[(const gchar*)val_name] = it.object([&](auto it){
      it["description"] = g_param_spec_get_blurb(pspec);
      GType type = G_PARAM_SPEC_VALUE_TYPE(pspec);
      if ( type == GIMP_TYPE_INT32 ||
           type == G_TYPE_INT ||
           type ==GIMP_TYPE_INT16) {
        it["type"]   = "integer";
        it["format"] = "int32";

      } else if (type == G_TYPE_UINT ||
                 type ==GIMP_TYPE_INT8) {
        it["type"] = "string";
        it["format"] = "byte";

      } else if(type ==G_TYPE_ENUM) {
        it["type"] = "integer";
        it["format"] = "int32";

      } else if(type ==G_TYPE_BOOLEAN) {
        it["type"] = "boolean";

      } else if (type ==G_TYPE_DOUBLE) {
        it["type"] = "number";
        it["format"] = "double";

      } else if (type ==G_TYPE_STRING) {
        it["type"] = "string";

      } else if (type ==GIMP_TYPE_RGB) {
        it["type"] = "array";
      } else if (type == GIMP_TYPE_INT32_ARRAY ||
                 type == GIMP_TYPE_INT16_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "integer";
          it["format"] = "int32";
        });
      } else if (type ==GIMP_TYPE_INT8_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
          it["format"] = "byte";
        });
      } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "number";
          it["format"] = "float";
        });
      } else if (type ==GIMP_TYPE_STRING_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
        });
      } else if (type ==GIMP_TYPE_COLOR_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
        });
      } else if (type ==GIMP_TYPE_ITEM_ID ||
                 type ==GIMP_TYPE_LAYER_ID ||
                 type ==GIMP_TYPE_CHANNEL_ID ||
                 type ==GIMP_TYPE_DRAWABLE_ID ||
                 type ==GIMP_TYPE_SELECTION_ID ||
                 type ==GIMP_TYPE_LAYER_MASK_ID ||
                 type ==GIMP_TYPE_VECTORS_ID ||
                 type ==GIMP_TYPE_DISPLAY_ID ||
                 type ==GIMP_TYPE_IMAGE_ID) {
        it["type"]   = "integer";
        it["format"] = "int32";
      } else if (type == GIMP_TYPE_PARASITE) {
        it["type"]   = "string";
      } else if (type == GIMP_TYPE_PDB_STATUS_TYPE) {
        it["type"]   = "string";
      }

    });
    return it;
  }

  template<typename F>
  void publish_site(JSON::Builder& builder, F path_publisher) {
    auto ibuilder = ref(builder);
    ibuilder = ibuilder.object([&](auto it){
      it["swagger"] = "2.0";

      it["info"] = it.object([&](auto it){
        it["title"] = "GIMP PDB";
        it["description"] = "GIMP PDB function call interface.";
        it["termsOfService"] = "";
        it["version"] =  "0.0.1";
      });

      it["schemes"] = it.array_with_values("http");
      it["host"] = "localhost:8920";
      it["basePath"] = "/api/v1/pdb";

      it["paths"] = it.object([&](auto it){
        path_publisher(it);
      });
    });

  }

  virtual void publish_one_proc(JSON::IBuilder& it, GimpProcedure* procedure) {
    GLib::CString path = g_strdup_printf("/%s", procedure->original_name);
    it[(const gchar*)path] = it.object([&](auto it){
      it["post"] = it.object([&](auto it) {
        it["operationId"] = procedure->original_name;
        it["description"] = procedure->blurb;

        it["consumes"] = it.array_with_values("application/json");
        it["produces"] = it.array_with_values("application/json");

        it["responses"]   = it.object([&](auto it) {
          it["200"] = it.object([&](auto it) {
            it["description"] = "Successive call";
            it["schema"] = it.object([&](auto it){
              it["type"]       = "object";
              it["properties"] = it.object([&](auto it) {

                it["context"] = it.object([&](auto it){
                  it["type"] = "object";
                  it["properties"] = it.object([&](auto it){
                    it["image"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["item"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["drawable"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["layer"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                  });
                });

                it["values"] = it.object([&](auto it){
                  it["type"] = "object";
                  it["properties"] = it.object([&](auto it){

                    bool use_named = use_named_values(procedure);
                    for(int i = 0; i < procedure->num_values; i ++) {
                      GParamSpec* pspec = procedure->values[i];
                      this->define_param_spec(it, use_named, pspec, i);
                    }

                  });
                });

              });
            });

          });

          it["400"] = it.object([&](auto it) {
            it["description"] = "Bad request.";
          });

          it["404"] = it.object([&](auto it) {
            it["description"] = "PDB function not found.";
            //
          });

          it["405"] = it.object([&](auto it) {
            it["description"] = "Method not allowed.";
          });

          it["500"] = it.object([&](auto it) {
            it["description"] = "Internal error.";
            //
          });

        });

        it["parameters"] = it.array([&](auto it){
          it = it.object([&](auto it){
            it["name"] = "payload";
            it["in"] = "body";

            it["schema"] = it.object([&](auto it){
              it["type"] = "object";

              it["properties"] = it.object([&](auto it) {

                it["context"] = it.object([&](auto it){
                  it["type"] = "object";
                  it["properties"] = it.object([&](auto it){
                    it["image"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["item"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["drawable"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                    it["layer"] = it.object([&](auto it){
                      it["type"]   = "integer";
                      it["format"] = "int32";
                    });
                  });
                });

                it["arguments"] = it.object([&](auto it){
                  it["type"] = "object";

                  it["properties"] = it.object([&](auto it){

                    bool use_named = use_named_args(procedure);
                    for(int i = 0; i < procedure->num_args; i ++) {
                      GParamSpec* pspec = procedure->args[i];
                      this->define_param_spec(it, use_named, pspec, i);
                    }

                  });
                });
              });

            });

          });
        });

      });

    });

  }

  JSON::INode publish(GimpProcedure* procedure) {
    JSON::Builder builder;
    publish_site(builder, [&](auto it){
      this->publish_one_proc(it, procedure);
    });

    JSON::INode result = JSON::ref(builder).get_root();
    return result;
  }

  JSON::INode publish(GimpPDB* pdb, gchar** procedures, gint size) {
    JSON::Builder builder;
    publish_site(builder, [&](auto it){
      auto ipdb = GLib::ref(pdb);

      for (int i = 0; i < size; i ++) {
        GimpProcedure* procedure = ipdb [gimp_pdb_lookup_procedure] (procedures[i]);
        if (procedure)
          this->publish_one_proc(it, procedure);
        else
          g_print("'%s' is not found.\n", procedures[i]);
      }

    });

    JSON::INode result = JSON::ref(builder).get_root();
    return result;
  }

};


///////////////////////////////////////////////////////////////////////
// OpenAPI v3.0 declaration

class ProcedurePublisherOAS3 : public ProcedurePublisher {
public:
  JSON::IBuilder& define_param_spec(JSON::IBuilder& it, bool use_named, GParamSpec* pspec, int index) {
    GLib::CString val_name = prop_name(pspec, use_named, index);
    it[(const gchar*)val_name] = it.object([&](auto it){
      GType type = G_PARAM_SPEC_VALUE_TYPE(pspec);
      it["description"] = g_param_spec_get_blurb(pspec);
      if ( type == GIMP_TYPE_INT32 ||
           type == G_TYPE_INT ||
           type ==GIMP_TYPE_INT16) {
        it["type"]   = "integer";
        it["format"] = "int32";

      } else if (type == G_TYPE_UINT ||
                 type ==GIMP_TYPE_INT8) {
        it["type"] = "string";
        it["format"] = "byte";

      } else if(type ==G_TYPE_ENUM) {
        it["type"] = "integer";
        it["format"] = "int32";

      } else if(type ==G_TYPE_BOOLEAN) {
        it["type"] = "boolean";

      } else if (type ==G_TYPE_DOUBLE) {
        it["type"] = "number";
        it["format"] = "double";

      } else if (type ==G_TYPE_STRING) {
        it["type"] = "string";

      } else if (type ==GIMP_TYPE_RGB) {
        it["type"] = "array";
      } else if (type == GIMP_TYPE_INT32_ARRAY ||
                 type == GIMP_TYPE_INT16_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "integer";
          it["format"] = "int32";
        });
      } else if (type ==GIMP_TYPE_INT8_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
          it["format"] = "byte";
        });
      } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "number";
          it["format"] = "float";
        });
      } else if (type ==GIMP_TYPE_STRING_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
        });
      } else if (type ==GIMP_TYPE_COLOR_ARRAY) {
        it["type"] = "array";
        it["items"] = it.object([&](auto it){
          it["type"] = "string";
        });
      } else if (type ==GIMP_TYPE_ITEM_ID ||
                 type ==GIMP_TYPE_LAYER_ID ||
                 type ==GIMP_TYPE_CHANNEL_ID ||
                 type ==GIMP_TYPE_DRAWABLE_ID ||
                 type ==GIMP_TYPE_SELECTION_ID ||
                 type ==GIMP_TYPE_LAYER_MASK_ID ||
                 type ==GIMP_TYPE_VECTORS_ID ||
                 type ==GIMP_TYPE_DISPLAY_ID ||
                 type ==GIMP_TYPE_IMAGE_ID) {
        it["type"]   = "integer";
        it["format"] = "int32";
      } else if (type == GIMP_TYPE_PARASITE) {
        it["type"]   = "string";
      } else if (type == GIMP_TYPE_PDB_STATUS_TYPE) {
        it["type"]   = "string";
      }
    });
    return it;
  }

  template<typename F>
  void publish_site(JSON::Builder& builder, F path_publisher) {
    auto ibuilder = ref(builder);
    ibuilder.with_object([&](auto it){
      it["openapi"] = "3.0.0";

      it["info"] = it.object([&](auto it){
        it["title"] = "GIMP PDB";
        it["description"] = "GIMP PDB function call interface.";
        it["termsOfService"] = "";
        it["version"] =  "0.0.1";
      });

      it["servers"] = it.array([&](auto it) {
        it = it.object([&](auto it){
          it["url"] = "http://localhost:8920/api/v1/pdb";
          it["description"] = "GIMP PDB interface endpoint";
        });
      });

      it["paths"] = it.object([&](auto it){
        path_publisher(it);
      });
    });

  }

  virtual void publish_one_proc(JSON::IBuilder& it, GimpProcedure* procedure) {
    GLib::CString path = g_strdup_printf("/%s", procedure->original_name);
    it[(const gchar*)path] = it.object([&](auto it){
      it["description"] = procedure->blurb;
      it["post"] = it.object([&](auto it) {
        it["operationId"] = procedure->original_name;
        it["responses"]   = it.object([&](auto it) {
          it["200"] = it.object([&](auto it) {
            it["description"] = "Successive call";
            it["content"]     = it.object([&](auto it) {
              it["application/json"] = it.object([&](auto it) {

                it["schema"] = it.object([&](auto it){
                  it["type"]       = "object";
                  it["properties"] = it.object([&](auto it) {

                    it["context"] = it.object([&](auto it){
                      it["type"] = "object";
                      it["additionalProperties"] = it.object([&](auto it){
                        it["image"] = it.object([&](auto it){
                          it["type"]   = "integer";
                          it["format"] = "int32";
                        });
                        it["item"] = it.object([&](auto it){
                          it["type"]   = "integer";
                          it["format"] = "int32";
                        });
                        it["drawable"] = it.object([&](auto it){
                          it["type"]   = "integer";
                          it["format"] = "int32";
                        });
                        it["layer"] = it.object([&](auto it){
                          it["type"]   = "integer";
                          it["format"] = "int32";
                        });
                      });
                    });

                    it["values"] = it.object([&](auto it){
                      it["type"] = "object";
                      it["properties"] = it.object([&](auto it){

                        bool use_named = use_named_values(procedure);
                        for(int i = 0; i < procedure->num_values; i ++) {
                          GParamSpec* pspec = procedure->values[i];
                          this->define_param_spec(it, use_named, pspec, i);
                        }

                      });
                    });

                  });
                });

              });
            });

          });

          it["400"] = it.object([&](auto it) {
            it["description"] = "Bad request parameter.";
            //
          });

          it["404"] = it.object([&](auto it) {
            it["description"] = "PDB function not found.";
            //
          });

          it["405"] = it.object([&](auto it) {
            it["description"] = "Method not allowed.";
          });

          it["500"] = it.object([&](auto it) {
            it["description"] = "Internal error.";
            //
          });

        });

        it["requestBody"] = it.object([&](auto it){

          it["content"] = it.object([&](auto it) {
            it["application/json"] = it.object([&](auto it) {
              it["schema"] = it.object([&](auto it){
                it["type"] = "object";
                it["properties"] = it.object([&](auto it) {

                  it["context"] = it.object([&](auto it){
                    it["type"] = "object";
                    it["additionalProperties"] = it.object([&](auto it){
                      it["image"] = it.object([&](auto it){
                        it["type"]   = "integer";
                        it["format"] = "int32";
                      });
                      it["item"] = it.object([&](auto it){
                        it["type"]   = "integer";
                        it["format"] = "int32";
                      });
                      it["drawable"] = it.object([&](auto it){
                        it["type"]   = "integer";
                        it["format"] = "int32";
                      });
                      it["layer"] = it.object([&](auto it){
                        it["type"]   = "integer";
                        it["format"] = "int32";
                      });
                    });
                  });

                  it["arguments"] = it.object([&](auto it){
                    it["type"] = "object";
                    it["properties"] = it.object([&](auto it){

                      bool use_named = use_named_args(procedure);
                      for(int i = 0; i < procedure->num_args; i ++) {
                        GParamSpec* pspec = procedure->args[i];
                        this->define_param_spec(it, use_named, pspec, i);
                      }

                    });
                  });

                });
              });
            });
          });

        });

      });
    });

  }

  JSON::INode publish(GimpProcedure* procedure) {
    JSON::Builder builder;
    publish_site(builder, [&](auto it){
      this->publish_one_proc(it, procedure);
    });

    JSON::INode result = JSON::ref(builder).get_root();
    return result;
  }

  JSON::INode publish(GimpPDB* pdb, gchar** procedures, gint size) {
    JSON::Builder builder;
    publish_site(builder, [&](auto it){
      auto ipdb = GLib::ref(pdb);

      for (int i = 0; i < size; i ++) {
        GimpProcedure* procedure = ipdb [gimp_pdb_lookup_procedure] (procedures[i]);
        if (procedure)
          this->publish_one_proc(it, procedure);
        else
          g_print("'%s' is not found.\n", procedures[i]);
      }

    });

    JSON::INode result = JSON::ref(builder).get_root();
    return result;
  }

};


///////////////////////////////////////////////////////////////////////
// Argument configurator interface

class JsonArgConfigurator {
public:
  RESTResource*  resource;
  SoupMessage*   message;

  GimpDrawable*  drawable;
  GimpImage*     image;
  Gimp*          gimp;
  GimpPDB*       pdb;
  GimpContext*   context;
  GimpObject*    display;
  GimpItem*      item;

  bool deserialize_args(GimpProcedure* procedure, GValueArray* args, JSON::INode args_json) {
    // Check whether argument ID is unique or not.
    bool use_object_based_args = ProcedurePublisher::use_named_args(procedure);

    // Set context parameter or json_parameter
    for (int i = 0; i < procedure->num_args; i ++) {
      // Overwrite context related arguments.
      GParamSpec* pspec = procedure->args[i];
      GLib::CString arg_name = ProcedurePublisher::prop_name(pspec, use_object_based_args, i);
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
              if (args_json.is_object()) {
                  gint val  = args_json[(const gchar*)arg_name];
                  g_value_set_int (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              resource->make_error_response(400,"Argument %s is not valid.", (const gchar*)arg_name);
              return false;
            }
          } else if (type ==G_TYPE_ENUM) {
          } else if (type ==G_TYPE_BOOLEAN) {
            try {
              if (args_json.is_object()) {
                  bool val  = args_json[(const gchar*)arg_name];
                  g_value_set_boolean (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              resource->make_error_response(400,"Argument %s is not valid.", (const gchar*)arg_name);
              return false;
            }
          } else if (type ==G_TYPE_DOUBLE) {
            try {
              if (args_json.is_object()) {
                  double val  = args_json[(const gchar*)arg_name];
                  g_value_set_double (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);

            } catch(JSON::INode::InvalidType e) {
              resource->make_error_response(400,"Argument %s is not valid.", (const gchar*)arg_name);
              return false;
            }
          } else if (type ==G_TYPE_STRING) {
            try {
              if (args_json.is_object()) {
                  const gchar* val  = args_json[(const gchar*)arg_name];
                  g_value_set_string (&args->values[i], val);
              }
            } catch(JSON::INode::InvalidIndex e) {
              g_print("Index %u is out of range\n", i);
            } catch(JSON::INode::InvalidType e) {
              resource->make_error_response(400,"Argument %s is not valid.", (const gchar*)arg_name);
              return false;
            }
          } else if (type ==GIMP_TYPE_RGB) {
          } else if (type ==GIMP_TYPE_INT32_ARRAY) {
            gsize   length;
            gint32* data;
            args_json[(const gchar*)arg_name].copy_values(&data, &length);
            gimp_value_set_int32array(&args->values[i], data, length);

          } else if (type ==GIMP_TYPE_INT16_ARRAY) {
            gsize   length;
            gint16* data;
            args_json[(const gchar*)arg_name].copy_values(&data, &length);
            gimp_value_set_int16array(&args->values[i], data, length);

          } else if (type ==GIMP_TYPE_INT8_ARRAY) {
            gsize   length;
            guint8* data;
            gimp_value_set_int8array(&args->values[i], data, length);

          } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
            gsize    length;
            gdouble* data;
            args_json[(const gchar*)arg_name].copy_values(&data, &length);
            gimp_value_set_floatarray(&args->values[i], data, length);

          } else if (type ==GIMP_TYPE_STRING_ARRAY) {
            using cpchar = const gchar*;
            gsize    length;
            cpchar*  data;
            args_json[(const gchar*)arg_name].copy_values(&data, &length);
            gimp_value_set_stringarray(&args->values[i], data, length);

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
              if (args_json.is_object()) {
                  gint id  = args_json[(const gchar*)arg_name];
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
        return false;
      }

    } // for
    return true;
  }

  bool configure_args(GimpProcedure* procedure, GValueArray* args, Gimp* gimp, JSON::INode ctx_json, JSON::INode args_json) {
    auto igimp     = GLib::ref( this->gimp = gimp );
    auto icontext  = GLib::ref( context    = igimp [gimp_get_user_context]() );
    auto iimage    = GLib::ref( image      = icontext [gimp_context_get_image]() );
    auto idrawable = GLib::ref( drawable   = GIMP_DRAWABLE(iimage [gimp_image_get_active_layer]()) );
    auto ipdb      = GLib::ref( pdb        = gimp->pdb );
    auto idisplay  = GLib::ref( display    = GIMP_OBJECT(icontext [gimp_context_get_display]()) );
    auto iitem     = GLib::ref( item       = GIMP_ITEM(drawable) );

    auto imessage  = GLib::ref( message );

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
    return deserialize_args(procedure, args, args_json);
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
// PDB Executor interface

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

  void serialize_values(GimpProcedure* procedure)
  {
    // Check whether argument ID is unique or not.
    bool use_object_based_values = ProcedurePublisher::use_named_values(procedure);;

    // Serialize return value
    result_json = JSON::build_object([&](auto it){

      for (int i = 0; i < procedure->num_values; i ++) {
        GParamSpec* pspec = procedure->values[i];
        GLib::CString val_name_ = ProcedurePublisher::prop_name(pspec, use_object_based_values, i);
        const gchar* val_name = val_name_;

        GType type = G_VALUE_TYPE(&result->values[i + 1]);
        if ( type == GIMP_TYPE_INT32 ||
             type == G_TYPE_INT ||
             type == G_TYPE_UINT ||
             type ==GIMP_TYPE_INT16 ||
             type ==GIMP_TYPE_INT8) {
          it[val_name] = g_value_get_int (&result->values[i + 1]);

        } else if(type ==G_TYPE_ENUM) {

        } else if(type ==G_TYPE_BOOLEAN) {
          it[val_name] = g_value_get_boolean (&result->values[i + 1]);

        } else if (type ==G_TYPE_DOUBLE) {
          it[val_name] = g_value_get_double (&result->values[i + 1]);

        } else if (type ==G_TYPE_STRING) {
          it[val_name] = g_value_get_string (&result->values[i + 1]);

        } else if (type ==GIMP_TYPE_RGB) {
          it[val_name] = JSON::IBuilder::null();

        } else if (type ==GIMP_TYPE_INT32_ARRAY) {
          it[val_name] = it.array_from(
              gimp_value_get_int32array(&result->values[i + 1]),
              gimp_value_get_array_length(&result->values[i + 1]) / sizeof(gint32));

        } else if (type ==GIMP_TYPE_INT16_ARRAY) {
          it[val_name] = it.array_from(
              gimp_value_get_int16array(&result->values[i + 1]),
              gimp_value_get_array_length(&result->values[i + 1]) / sizeof(gint16));

        } else if (type ==GIMP_TYPE_INT8_ARRAY) {
          it[val_name] = it.array_from(
              gimp_value_get_int8array(&result->values[i + 1]),
              gimp_value_get_array_length(&result->values[i + 1]) / sizeof(guint8));

        } else if (type ==GIMP_TYPE_FLOAT_ARRAY) {
          it[val_name] = it.array_from(
              gimp_value_get_floatarray(&result->values[i + 1]),
              gimp_value_get_array_length(&result->values[i + 1]) / sizeof(gdouble));

        } else if (type ==GIMP_TYPE_STRING_ARRAY) {
          it[val_name] = it.array_from(
              gimp_value_get_stringarray(&result->values[i + 1]),
              gimp_value_get_array_length(&result->values[i + 1]) / sizeof(const gchar*));

        } else if (type ==GIMP_TYPE_COLOR_ARRAY) {
          it[val_name] = JSON::IBuilder::null();

        } else if (type ==GIMP_TYPE_ITEM_ID ||
                   type ==GIMP_TYPE_LAYER_ID ||
                   type ==GIMP_TYPE_CHANNEL_ID ||
                   type ==GIMP_TYPE_DRAWABLE_ID ||
                   type ==GIMP_TYPE_SELECTION_ID ||
                   type ==GIMP_TYPE_LAYER_MASK_ID ||
                   type ==GIMP_TYPE_VECTORS_ID ||
                   type ==GIMP_TYPE_DISPLAY_ID ||
                   type ==GIMP_TYPE_IMAGE_ID) {
          it[val_name] = g_value_get_int(&result->values[i + 1]);

        } else if (type == GIMP_TYPE_PARASITE) {
          it[val_name] = JSON::IBuilder::null();

        } else if (type == GIMP_TYPE_PDB_STATUS_TYPE) {
          it[val_name] = JSON::IBuilder::null();
        }

      }

    });
  }

  void execute(GimpProcedure* procedure, Gimp* gimp, GimpContext* context,
               GimpProgress* progress, GValueArray* args, GimpObject* display)
  {
    PDBSyncExecutor::execute(procedure, gimp, context, progress, args, display);
    serialize_values(procedure);
  }
};

using JsonPDBRunner = ProcedureRunnerImpl<JsonArgConfigurator, JsonPDBSyncExecutor>;

///////////////////////////////////////////////////////////////////////
// PDB REST interface

void RESTPDB::get()
{
  using Publisher = ProcedurePublisherOAS2;

  const gchar* proc_name = matched->data()["name"];
  g_print("proc_name=%s\n", proc_name);
  auto imessage = GLib::ref(message);

  if (!proc_name) {
    auto             pdb      = GLib::ref( gimp->pdb );
    gint             num_procs;
    gchar**          procs = NULL;
    gboolean         query = pdb [gimp_pdb_query] (".*", ".*", ".*", ".*", ".*", ".*", ".*", &num_procs, &procs, NULL);

    Publisher publisher;
    JSON::INode result_json = publisher.publish(gimp->pdb, procs, num_procs);
    make_json_response(200, result_json.ptr());
    soup_message_headers_append(message->response_headers, "Access-Control-Allow-Origin", "*");
  } else {
    auto           pdb       = GLib::ref( gimp->pdb );
    GimpProcedure* procedure = pdb [gimp_pdb_lookup_procedure] (proc_name);

    if (procedure) {
      Publisher publisher;
      JSON::INode result_json = publisher.publish(procedure);
      make_json_response(200, result_json.ptr());
      soup_message_headers_append(message->response_headers, "Access-Control-Allow-Origin", "*");

    } else {
      g_print("invalid procedure name.\n");
      make_error_response(404, "%s is not found.", (const gchar*)proc_name);
    }
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

      auto arg_conf = runner.get_arg_configurator();
      auto executor = runner.get_executor();

      arg_conf->message  = message;
      arg_conf->resource = this;

      if (!runner.run(gimp, ctx_node, arg_node))
        return;

      JSON::INode new_root = JSON::build_object([&](auto it) {
        it["context"] = arg_conf->get_context_json();
        it["values"]  = executor->result_json;
        executor->result_json   = NULL;
      });
      make_json_response(200, new_root.ptr());
    } else {
      g_print("invalid procedure name.\n");
      make_error_response(404, "%s is not found.", (const gchar*)proc_name);
    }

  } catch(JSON::INode::InvalidType e) {
  } catch(JSON::INode::InvalidIndex e) {

  }
}


void RESTPDB::del()
{

}
