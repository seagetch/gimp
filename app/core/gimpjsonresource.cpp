/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpJsonResource
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
__DECLARE_GTK_CLASS__(GObject, G_TYPE_OBJECT);
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"
#include "core-enums.h"

#include "gimp.h"

#include "gimpcontext.h"
#include "gimpcontainer.h"

#include "gimp-intl.h"

#include "core/gimpparamspecs.h"
}

#include "gimpjsonresource.h"

using namespace GLib;

class JsonResourceSingleton;

class JsonResource : virtual public ImplBase, virtual public JsonResourceInterface
{
protected:
  Object<JsonNode> root;
  friend class JsonResourceSingleton;
public:

  static void class_init(Traits<GimpJsonResource>::Class* klass);

  JsonResource(GObject* o) : ImplBase(o), root(NULL) { };
  virtual ~JsonResource() { };

  //////////////////////////////////////////////
  // Public interface

  virtual JsonNode* get_json     ();
  virtual void      set_json     (JsonNode* node);

  //////////////////////////////////////////////
  // Implementation
  virtual void      set_property (guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec);
  virtual void      get_property (guint         property_id,
                                  GValue       *value,
                                  GParamSpec   *pspec);
};


extern const char gimp_json_resource_name[] = "GimpJsonResource";
using JsonResourceClass = GClass<gimp_json_resource_name,
                                 UseCStructs<GimpData, GimpJsonResource>,
                                 JsonResource>;
static JsonResourceClass class_instance;

#define _override(method) JsonResourceClass::__(&klass->method).bind<&Impl::method>()
void JsonResource::class_init(Traits<GimpJsonResource>::Class* klass)
{
  using Impl = JsonResource;
//  class_instance.with_class(klass)->
//      as_class<GObject>([](GObjectClass* klass){
//      });
}
JsonNode*
JsonResource::get_json()
{
  return root;
}

void
JsonResource::set_json(JsonNode* node)
{
  if (node != root) {
    root = node;
    ref(g_object) [g_object_notify] ("tool-options");
  }
}

GimpJsonResource*
JsonResourceFactoryBase::load (const gchar* filename)
{
  auto parser = ref(json_parser_new());
  JsonNode* root;
  GError* jerror = NULL;
  json_parser_load_from_file(parser, filename, &jerror);
  if (jerror) {
    return NULL;
  }
  root = json_parser_get_root(parser);
  auto json_res = GIMP_JSON_RESOURCE(JsonResourceInterface::new_instance());
  auto json_impl = JsonResourceInterface::cast(json_res);
  json_impl->set_json(root);
  return json_res;
}

void
JsonResourceFactoryBase::save (const gchar*  filename,
                               GimpJsonResource* json_res)
{
  auto json_impl = JsonResourceInterface::cast(json_res);
  auto root = ref(json_impl->get_json());

}

GimpDataFactory*
JsonResourceFactoryBase::create_data_factory ()
{
#if 0
  static const GimpDataFactoryLoaderEntry tool_preset_loader_entries[] =
  {
    { gimp_tool_preset_load,     GIMP_TOOL_PRESET_FILE_EXTENSION,     TRUE  }
  };

  gimp->tool_preset_factory =
    gimp_data_factory_new (gimp,
                           GIMP_TYPE_TOOL_PRESET,
                           "tool-preset-path", "tool-preset-path-writable",
                           tool_preset_loader_entries,
                           G_N_ELEMENTS (tool_preset_loader_entries),
                           gimp_tool_preset_new,
                           NULL);
#endif
  return NULL;
}

GObject*
JsonResourceFactoryBase::create_view ()
{
  return NULL;
}

GObject*
JsonResourceFactoryBase::create_editor ()
{
  return NULL;
}

////////////////////////////////////////////////////////////////////////////
// C++ interfaces
GimpData*
JsonResourceInterface::new_instance () {
  return NULL;
}

JsonResourceInterface*
JsonResourceInterface::cast(gpointer obj) {
  return dynamic_cast<JsonResourceInterface*>(JsonResourceClass::get_private(obj));
}

bool JsonResourceInterface::is_instance(gpointer obj) {
  return JsonResourceClass::Traits::is_instance(obj);
}
////////////////////////////////////////////////////////////////////////////
// C compatibility functions
GType gimp_json_resource_get_type() { return JsonResourceClass::get_type(); }
