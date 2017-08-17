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

#include "core-types.h"
#include "core-enums.h"

#include "gimp.h"

#include "gimpcontext.h"
#include "gimpcontainer.h"
#include "gimpdata.h"

#include "gimp-intl.h"

#include "core/gimpparamspecs.h"
#include "config/gimpcoreconfig.h"
}
#include "gimpjsonresource.h"
#include <type_traits>


#include "presets/layer-preset.h"


using namespace GLib;

class JsonResourceSingleton;
GLib::GClassBase<UseCStructs<GimpBaseConfig, GimpCoreConfig> > core_class;

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
using JsonResourceClass = GClass<gimp_json_resource_name,
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


////////////////////////////////////////////////////////////////////////////
// JsonResourceConfig

const gchar*
JsonResourceConfig::ConfigExtender::get_readonly_paths ()
{
  g_print("get::readonly path\n");
  return readonly_path;
}
const gchar*
JsonResourceConfig::ConfigExtender::get_writable_paths ()
{
  g_print("get::writable path\n");
  return writable_path;
}
void
JsonResourceConfig::ConfigExtender::set_readonly_paths (const gchar* p)
{
  g_print("set::readonly path to `%s'\n", p);
  readonly_path = g_strdup(p);
}
void
JsonResourceConfig::ConfigExtender::set_writable_paths (const gchar* p)
{
  g_print("set::writable path to `%s'\n", p);
  writable_path = g_strdup(p);
}


JsonResourceConfig::ConfigExtender*
JsonResourceConfig::get_extender_for(GObject* obj)
{
  CString type_key         = g_strdup_printf("config_extend_%p", &typeid(*this));
  ConfigExtender* extender = (JsonResourceConfig::ConfigExtender*)(g_object_get_data(obj, type_key) );
  if (!extender) {
    g_print("Install ConfigExtender.\n");
    extender = get_extender();
    g_object_set_cxx_object(obj, type_key, extender);
  }
  return extender;
}


void
JsonResourceConfig::extend (gpointer klass)
{
  const gchar* readonly_prop = get_readonly_path_prop_name();
  const gchar* writable_prop = get_writable_path_prop_name();
  const gchar* readonly_path = get_readonly_default_paths();
  const gchar* writable_path = get_writable_default_paths();

  g_print("  Installing %s with default = %s\n", readonly_prop, readonly_path);
  g_print("  Installing %s with default = %s\n", writable_prop, writable_path);
  core_class.with_class(klass)->
      install_property(gimp_param_spec_config_path (
                           readonly_prop, NULL, NULL,
                           GIMP_CONFIG_PATH_DIR_LIST, readonly_path,
                           GParamFlags(GIMP_PARAM_STATIC_STRINGS |GIMP_CONFIG_PARAM_RESTART |GIMP_CONFIG_PARAM_FLAGS)),
                       [this] (GObject* obj) -> Value { // getter
                         auto extender = this->get_extender_for(obj);
                         return extender->get_readonly_paths();
                       },
                       [this] (GObject* obj, Value src) { // setter
                         auto extender = this->get_extender_for(obj);
                         extender->set_readonly_paths(src);
                       }
                       )->
      install_property(gimp_param_spec_config_path (
                           writable_prop, NULL, NULL,
                           GIMP_CONFIG_PATH_DIR_LIST, writable_path,
                           GParamFlags(GIMP_PARAM_STATIC_STRINGS |GIMP_CONFIG_PARAM_RESTART |GIMP_CONFIG_PARAM_FLAGS)),
                       [this] (GObject* obj) -> Value { // getter
                         auto extender = this->get_extender_for(obj);
                         return extender->get_writable_paths();
                       },
                       [this] (GObject* obj, Value src) { // setter
                         auto extender = this->get_extender_for(obj);
                         extender->set_writable_paths(src);
                       }
                       );
}


////////////////////////////////////////////////////////////////////////////
// JsonResourceFactory

static JsonResourceFactory* singleton = NULL;
JsonResourceFactory* get_json_resource_factory() {
  if (!singleton)
    singleton = new JsonResourceFactory();
  return singleton;
}
//GLib::HashTable JsonResourceFactory factory;
GLib::IHashTable<gconstpointer, JsonResourceConfig*> JsonResourceFactory::registry()
{
  if (!_registry)
    _registry = g_hash_table_new(NULL, NULL);
  return ref<gconstpointer, JsonResourceConfig*>(_registry);
}

void JsonResourceFactory::register_config (JsonResourceConfig* config)
{
  gconstpointer type_ptr = &typeid(config);
  registry().insert(type_ptr, config);
}


void JsonResourceFactory::entrypoint(Gimp* gimp)
{
  g_signal_connect_delegator (G_OBJECT(gimp), "initialize", Delegators::delegator(this, &JsonResourceFactory::on_initialize));
  g_signal_connect_delegator (G_OBJECT(gimp), "restore",    Delegators::delegator(this, &JsonResourceFactory::on_restore));
  g_signal_connect_delegator (G_OBJECT(gimp), "exit",       Delegators::delegator(this, &JsonResourceFactory::on_exit));

  GimpCoreConfigClass* klass = GIMP_CORE_CONFIG_CLASS (g_type_class_ref(GIMP_TYPE_CORE_CONFIG));

  register_config( LayerPresetConfig::layer_preset_config() );

  registry().each([&](auto key, auto it) {
    it->extend(klass);
  });
  g_type_class_unref(klass);
}


GList*
JsonResourceFactory::load (GimpContext* context,
                               const gchar* filename,
                               GError**     error)
{
  CString basename  = g_path_get_basename (filename);
  CString res_name = g_strndup(basename, strlen(basename) - strlen(".json"));
  auto json_res = GIMP_JSON_RESOURCE(JsonResourceInterface::new_instance(context, res_name));
  auto json_impl = JsonResourceInterface::cast(json_res);
  json_impl->load(context, filename, error);
  return g_list_prepend(NULL, json_res);
}


GimpDataFactory*
JsonResourceFactory::create_data_factory (Gimp* gimp, JsonResourceConfig* config)
{
  const gchar* readonly_prop = config->get_readonly_path_prop_name();
  const gchar* writable_prop = config->get_writable_path_prop_name();
  static GimpDataFactoryLoaderEntry entries[] = {
      {
        JsonResourceFactory::load, //load function
        ".json", // extension
        writable_prop != NULL // writable
      }
  };

  GimpDataFactory* result =
      gimp_data_factory_new (gimp,
                             JsonResourceClass::get_type(),
                             readonly_prop, writable_prop,
                             entries,
                             G_N_ELEMENTS (entries),
                             JsonResourceInterface::new_instance,
                             NULL);
  return result;
}

void
JsonResourceFactory::on_initialize (Gimp               *gimp,
                                    GimpInitStatusFunc  status_callback)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = this->create_data_factory(gimp, it);
    it->set_data_factory(factory);
  });
}


void
JsonResourceFactory::on_restore (Gimp               *gimp,
                                 GimpInitStatusFunc  status_callback)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = it->get_data_factory();
    gimp_data_factory_data_init (factory, gimp->user_context,
                                 gimp->no_data);
  });
}


gboolean
JsonResourceFactory::on_exit(Gimp *gimp,
                             gboolean force)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = it->get_data_factory();
    gimp_data_factory_data_save (factory);
  });
  return FALSE;
}


void      json_resource_factory_entry_point (Gimp* gimp)
{
  get_json_resource_factory()->entrypoint(gimp);
}


