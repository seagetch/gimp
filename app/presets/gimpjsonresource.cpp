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
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "core/core-types.h"
#include "core/core-enums.h"

#include "core/gimp.h"

#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpdata.h"

#include "gimp-intl.h"

#include "core/gimpparamspecs.h"
#include "config/gimpcoreconfig.h"
}
#include "gimpjsonresource.h"



using namespace GLib;

class JsonResource : virtual public ImplBase, virtual public JsonResourceInterface
{
protected:
  Object<JsonNode> root;
public:

  static void class_init(Traits<GimpJsonResource>::Class* klass);

  JsonResource(GObject* o) : ImplBase(o), root(NULL) { };
  virtual ~JsonResource() { };

  //////////////////////////////////////////////
  // Implementation
  virtual gboolean     save           (GError** error);
  virtual const gchar* get_extension  () { return ".json"; };
  virtual GimpData*    duplicate      ();

  virtual gboolean     load           (GimpContext* context,
                                       const gchar* filename,
                                       GError** error);

  //////////////////////////////////////////////
  // Public interface

  virtual JsonNode* get_json     ();
  virtual void      set_json     (JsonNode* node);
};


extern const char gimp_json_resource_name[] = "GimpJsonResource";
using JsonResourceClass = NewGClass<gimp_json_resource_name,
                                 UseCStructs<GimpData, GimpJsonResource>,
                                 JsonResource>;
static JsonResourceClass class_instance;

#define _override(method) JsonResourceClass::__(&klass->method).bind<&Impl::method>()
void JsonResource::class_init(Traits<GimpJsonResource>::Class* klass)
{
  using Impl = JsonResource;
  class_instance.with_class(klass)->
      as_class<GimpData>([](GimpDataClass* klass) {
        _override (save);
        _override (get_extension);
        _override (duplicate);
      })->
      install_property(
          g_param_spec_boxed("root", NULL, NULL,JSON_TYPE_NODE , GParamFlags(GIMP_PARAM_READWRITE|G_PARAM_CONSTRUCT)),
          [](GObject* obj)->Value {
            auto resource = JsonResourceInterface::cast(obj);
            return resource->get_json();
          },
          [](GObject* obj, Value src)->void {
            auto resource = JsonResourceInterface::cast(obj);
            resource->set_json((JsonNode*)((GObject*)src));
          });
}

gboolean
JsonResource::load (GimpContext* context,
                    const gchar* filename,
                    GError**     error)
{
  auto parser = ref(json_parser_new());
  JsonNode* root;
  parser [json_parser_load_from_file] (filename, error);
  if (error) {
    return FALSE;
  }
  root = json_parser_get_root(parser);
  set_json(root);
  return TRUE;
}

gboolean
JsonResource::save(GError** error)
{
  const gchar* filename = ref(g_object) [gimp_data_get_filename] ();
  auto root = ref(get_json());
  auto generator = ref(json_generator_new());
  generator [json_generator_set_root] (root);
  generator [json_generator_to_file] (filename, error);
  return (error != NULL);
}

GimpData*
JsonResource::duplicate()
{
  auto self = ref(g_object);
  const gchar* name = self [gimp_object_get_name] ();
  auto result = JsonResourceInterface::new_instance(NULL, name);
  auto impl   = JsonResourceInterface::cast(result);
  JsonNode* root = get_json();
  JsonNode* copy = json_node_copy(root);
  impl->set_json(copy);
  return GIMP_DATA(result);
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
    ref(g_object) [g_object_notify] ("root");
  }
}

////////////////////////////////////////////////////////////////////////////
// C++ interfaces
GimpData*
JsonResourceInterface::new_instance (GimpContext* context, const gchar* name) {
  return GIMP_DATA( g_object_new(JsonResourceClass::Traits::get_type(),
                                 "name", name,
                                 "mime-type", "application/json",
                                 NULL) );
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
GType gimp_json_resource_get_type()
{
  return JsonResourceClass::get_type();
}
GimpData* gimp_json_resource_new (GimpContext* context, const gchar* name)
{
  return JsonResourceInterface::new_instance(context, name);
}
