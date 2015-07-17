/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpbrush-load.c
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

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifndef _O_BINARY
#define _O_BINARY 0
#endif

#include <glib-object.h>
#include <glib/gstdio.h>
#include <cairo.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpcolor/gimpcolor.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include <glib.h> /* GIOChannel */
#include <glib/gprintf.h> /* strip / split */


#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpmypaintbrush.h"
#include "gimpmypaintbrush-save.h"
#include "mypaintbrush-brushsettings.h"

#include "gimp-intl.h"
#include <jansson.h>

}

#include "gimpmypaintbrush-private.hpp"
#include "base/glib-cxx-utils.hpp"

#define GIMP_MYPAINT_BRUSH_FILE_EXTENSION        ".myb"
#define GIMP_MYPAINT_BRUSH_ICON_FILE_EXTENSION "_prev.png"

#define CURRENT_BRUSHFILE_VERSION 3

/*  local function prototypes  */

class MypaintBrushWriter {
  private:
  GimpMypaintBrush* source;
  gint64            version;
  
  void write_file (GError **error);
  json_t* build_json ();
  bool is_default(gchar* name);
  gchar *quote        (const gchar *string);
  void save_icon       (gchar *filename);
  void dump();
  
  public:
  MypaintBrushWriter(GimpMypaintBrush* brush);
  ~MypaintBrushWriter();
  bool save_brush (GError **error);
};

extern "C" {
/*  public functions  */

gboolean
gimp_mypaint_brush_save (GimpData* data,
                         GError      **error)
{
  GimpMypaintBrush* brush = GIMP_MYPAINT_BRUSH(data);

  g_return_val_if_fail (error == NULL || *error == NULL, false);
  
  MypaintBrushWriter writer(brush);
  return writer.save_brush (error);
}

}

/*  private functions  */


MypaintBrushWriter::MypaintBrushWriter(GimpMypaintBrush* brush)
  : source(brush), version(CURRENT_BRUSHFILE_VERSION)
{
  g_object_ref(source);
}

MypaintBrushWriter::~MypaintBrushWriter()
{
  if (source)
    g_object_unref (source);
}

bool
MypaintBrushWriter::save_brush (GError      **error)
{
  const gchar* filename = gimp_data_get_filename(GIMP_DATA(source));
  if (!filename)
    return false;

  StringHolder dirname(g_path_get_dirname (filename));
  StringHolder basename(g_path_get_basename (filename));
  StringHolder brushname(g_strndup(basename.ptr(), strlen(basename.ptr()) - strlen(GIMP_MYPAINT_BRUSH_FILE_EXTENSION)));
  StringHolder icon_filename(g_strconcat(brushname.ptr(), GIMP_MYPAINT_BRUSH_ICON_FILE_EXTENSION, NULL));
  StringHolder icon_fullpath(g_build_filename(dirname.ptr(), icon_filename.ptr(), NULL));

  ScopeGuard<FILE, int(FILE*)> file(g_fopen (filename, "wb"), fclose);
  ScopeGuard<json_t, void(json_t*)> result(build_json(), json_decref);

  json_dumpf(result.ptr(), file.ptr(), 0);

  g_print ("Write Icon: %s\n", icon_fullpath.ptr());
  save_icon (icon_fullpath.ptr());
  return true;
}

bool
MypaintBrushWriter::is_default(gchar* name)
{
  GHashTableHolder<gchar*, MyPaintBrushSettings*> settings_dict(mypaint_brush_get_brush_settings_dict ());
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);

  MyPaintBrushSettings* setting = settings_dict[name];

  if (!setting)
    return false;
    
  float base_value = priv->get_base_value(setting->index);
  if (base_value != setting->default_value)
    return false;
  
  return true;
}

json_t*
MypaintBrushWriter::build_json()
{
  g_return_val_if_fail(source != NULL, NULL);
  
  const gchar* filename = gimp_data_get_filename (GIMP_DATA(source));
  
  GListHolder settings(mypaint_brush_get_brush_settings ());
  GListHolder switches(mypaint_brush_get_brush_switch_settings ());
  GListHolder texts(mypaint_brush_get_brush_text_settings ());
  GListHolder inputs(mypaint_brush_get_input_settings ());
  gchar* brush_name;
  g_object_get(source, "name",&brush_name, NULL);
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);

  json_t* result = json_object();
  // version
  json_object_set_new(result, "version", json_integer(version));
  // group
  json_object_set_new(result, "group", json_string(priv->get_group()));
  // parent_brush_name
  json_object_set_new(result, "parent_brush_name", json_string(brush_name));
  // settings
  json_t* result_settings = json_object();
  json_object_set_new(result, "settings", result_settings);

  for (GList* item = settings.ptr(); item; item = item->next) {
    MyPaintBrushSettings* setting = reinterpret_cast<MyPaintBrushSettings*>(item->data);
    json_t* value = json_object();
    json_object_set_new(result_settings, setting->internal_name, value);

    GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
    g_print("BASE_VALUE(%s)=%f\n", setting->internal_name, priv->get_base_value(setting->index));
    json_object_set_new(value, "base_value", json_real(priv->get_base_value(setting->index)));
    
    json_t* result_inputs = json_object();
    json_object_set_new(value, "inputs", result_inputs);
    
    if (v->mapping) {
      for (GList* input_iter = inputs.ptr(); input_iter; input_iter = input_iter->next) {
        MyPaintBrushInputSettings* input = reinterpret_cast<MyPaintBrushInputSettings*>(input_iter->data);
        int n = v->mapping->get_n(input->index);

        if (n == 0)
          continue;

        json_t* input_array = json_array();
        json_object_set_new(result_inputs, input->name, input_array);
        
        for (int i = 0; i < n; i ++) {
          float x, y;
          v->mapping->get_point(input->index, i, &x, &y);
          json_t* val_pair = json_array();
          json_array_append(val_pair, json_real(x));
          json_array_append(val_pair, json_real(y));
          json_array_append(input_array, val_pair);
        }
      }
    }
  }
  // switches
  json_t* result_switches = json_object();
  json_object_set_new(result, "switches", result_switches);

  for (GList* item = switches.ptr(); item; item = item->next) {
    MyPaintBrushSwitchSettings* setting = reinterpret_cast<MyPaintBrushSwitchSettings*>(item->data);
    bool value = priv->get_bool_value(setting->index);
    json_object_set_new(result_switches, setting->internal_name, json_boolean(value));
  }
  // text

  return result;
}


void
MypaintBrushWriter::write_file (GError** error)
{
  g_return_if_fail(source != NULL);
  
  const gchar* filename = gimp_data_get_filename (GIMP_DATA(source));
  
  GListHolder settings(mypaint_brush_get_brush_settings ());
  GListHolder switches(mypaint_brush_get_brush_switch_settings ());
  GListHolder texts(mypaint_brush_get_brush_text_settings ());
  GListHolder inputs(mypaint_brush_get_input_settings ());
  gchar* brush_name;
  g_object_get(source, "name",&brush_name, NULL);
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);
  ScopeGuard<FILE, int(FILE*)> file(g_fopen(filename,"w"), fclose);
//  FILE* f = file.ptr();
  FILE* f = stdout;

  fprintf(f, "version %ld\n", version);

  for (GList* item = settings.ptr(); item; item = item->next) {
    MyPaintBrushSettings* setting = reinterpret_cast<MyPaintBrushSettings*>(item->data);
    GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
    fprintf(f, "%s %f ", setting->internal_name, v->base_value);
    if (v->mapping) {
      for (GList* input_iter = inputs.ptr(); input_iter; input_iter = input_iter->next) {
        fprintf(f, " |");
        MyPaintBrushInputSettings* input = reinterpret_cast<MyPaintBrushInputSettings*>(input_iter->data);
        int n = v->mapping->get_n(input->index);
        if (n == 0)
          continue;
        fprintf(f, "%s", input->name);
        float x, y;
        v->mapping->get_point(input->index, 0, &x, &y);
        fprintf(f, " (%f,%f)", x, y);
        for (int i = 1; i < n; i ++) {
          v->mapping->get_point(input->index, i, &x, &y);
          fprintf(f, ", (%f,%f)", x, y);
        }
      }
    }
    fprintf(f, "\n");
  }
  for (GList* item = switches.ptr(); item; item = item->next) {
    MyPaintBrushSwitchSettings* setting = reinterpret_cast<MyPaintBrushSwitchSettings*>(item->data);
    bool value = priv->get_bool_value(setting->index);
    fprintf(f, "%s %d", setting->internal_name, value? 1:0);
    fprintf(f, "\n");
  }
}

gchar* 
MypaintBrushWriter::quote (const gchar *string)
{
  return g_uri_escape_string (string, NULL, TRUE);
}

void
MypaintBrushWriter::save_icon (gchar *filename)
{
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);
  cairo_surface_t* icon_surface = priv->get_icon_image();
  cairo_surface_write_to_png(icon_surface, filename);
}
