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
#include <json-glib/json-glib.h>

}

#include "gimpmypaintbrush-private.hpp"
#include "base/glib-cxx-utils.hpp"

#define GIMP_MYPAINT_BRUSH_FILE_EXTENSION        ".myb"
#define GIMP_MYPAINT_BRUSH_ICON_FILE_EXTENSION "_prev.png"

#define CURRENT_BRUSHFILE_VERSION 3

using namespace GLib;

/*  local function prototypes  */

class MypaintBrushWriter {
private:
  GLib::ObjectWrapper<GimpMypaintBrush> source;
  gint64            version;
  
  void write_file (GError **error);
  JsonNode* build_json ();
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
}

MypaintBrushWriter::~MypaintBrushWriter()
{
}

bool
MypaintBrushWriter::save_brush (GError      **error)
{
  const gchar* filename = gimp_data_get_filename(GIMP_DATA(source.ptr()));
  if (!filename)
    return false;

  CString dirname       = g_path_get_dirname (filename);
  CString basename      = g_path_get_basename (filename);
  CString brushname(g_strndup(basename.ptr(), strlen(basename.ptr()) - strlen(GIMP_MYPAINT_BRUSH_FILE_EXTENSION)));
  CString icon_filename(g_strconcat(brushname.ptr(), GIMP_MYPAINT_BRUSH_ICON_FILE_EXTENSION, NULL));
  CString icon_fullpath(g_build_filename(dirname.ptr(), icon_filename.ptr(), NULL));

//  ScopedPointer<FILE, int(FILE*), fclose> file(g_fopen (filename, "wb"));
  ObjectWrapper<JsonGenerator> generator = json_generator_new();
  JsonNode* root = build_json();
  json_generator_set_root(generator, root);
  json_generator_to_file(generator, filename, NULL);
  json_node_unref(root);

  g_print ("Write Icon: %s\n", icon_fullpath.ptr());
  save_icon (icon_fullpath.ptr());
  return true;
}

bool
MypaintBrushWriter::is_default(gchar* name)
{
  HashTable<gchar*, MyPaintBrushSettings*> settings_dict(mypaint_brush_get_brush_settings_dict ());
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);

  MyPaintBrushSettings* setting = settings_dict[name];

  if (!setting)
    return false;
    
  float base_value = priv->get_base_value(setting->index);
  if (base_value != setting->default_value)
    return false;
  
  return true;
}

JsonNode*
MypaintBrushWriter::build_json()
{
  g_return_val_if_fail(source.ptr() != NULL, NULL);
  ObjectWrapper<JsonBuilder> builder = json_builder_new();
  json_builder_begin_object(builder);
  
  const gchar* filename = gimp_data_get_filename (GIMP_DATA(source.ptr()));
  
  List settings = mypaint_brush_get_brush_settings ();
  List switches = mypaint_brush_get_brush_switch_settings ();
  List texts    = mypaint_brush_get_brush_text_settings ();
  List inputs   = mypaint_brush_get_input_settings ();
  CString brush_name = g_strdup(source.get("name"));
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);

  // version
  json_builder_set_member_name(builder, "version");
  json_builder_add_int_value(builder, version);
  // group
  json_builder_set_member_name(builder, "group");
  json_builder_add_string_value(builder, priv->get_group());
  // parent_brush_name
  json_builder_set_member_name(builder, "parent_brush_name");
  json_builder_add_string_value(builder, brush_name);
  // settings
  json_builder_set_member_name(builder, "settings");
  json_builder_begin_object(builder);

  for (GList* item = settings.ptr(); item; item = item->next) {
    MyPaintBrushSettings* setting = reinterpret_cast<MyPaintBrushSettings*>(item->data);
    json_builder_set_member_name(builder, setting->internal_name);
    json_builder_begin_object(builder);
    
    GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
    g_print("BASE_VALUE(%s)=%f\n", setting->internal_name, priv->get_base_value(setting->index));
    json_builder_set_member_name(builder, "base_value");
    json_builder_add_double_value(builder, priv->get_base_value(setting->index));
    
    json_builder_set_member_name(builder, "inputs");
    json_builder_begin_object(builder);
    
    if (v->mapping) {
      for (GList* input_iter = inputs.ptr(); input_iter; input_iter = input_iter->next) {
        MyPaintBrushInputSettings* input = reinterpret_cast<MyPaintBrushInputSettings*>(input_iter->data);
        int n = v->mapping->get_n(input->index);

        if (n == 0)
          continue;

        json_builder_set_member_name(builder, input->name);
        json_builder_begin_array(builder);
        
        for (int i = 0; i < n; i ++) {
          float x, y;
          v->mapping->get_point(input->index, i, &x, &y);
          json_builder_begin_array(builder);
          json_builder_add_double_value(builder, x);
          json_builder_add_double_value(builder, y);
          json_builder_end_array(builder);
        }
        json_builder_end_array(builder);
      }
    }
    json_builder_end_object(builder);
    json_builder_end_object(builder);
  }
  json_builder_end_object(builder);

  // switches
  json_builder_set_member_name(builder, "switches");
  json_builder_begin_object(builder);
  for (GList* item = switches.ptr(); item; item = item->next) {
    MyPaintBrushSwitchSettings* setting = reinterpret_cast<MyPaintBrushSwitchSettings*>(item->data);
    bool value = priv->get_bool_value(setting->index);
    json_builder_set_member_name(builder, setting->internal_name);
    json_builder_add_boolean_value(builder, value);
  }
  json_builder_end_object(builder);

  // text
  json_builder_set_member_name(builder, "texts");
  json_builder_begin_object(builder);

  for (GList* item = texts.ptr(); item; item = item->next) {
    MyPaintBrushTextSettings* setting = reinterpret_cast<MyPaintBrushTextSettings*>(item->data);
    char* value = priv->get_text_value(setting->index);
    json_builder_set_member_name(builder, setting->internal_name);
    json_builder_add_string_value(builder, value);
  }
  json_builder_end_object(builder);
  json_builder_end_object(builder);

  return json_builder_get_root(builder); 
}


void
MypaintBrushWriter::write_file (GError** error)
{
  g_return_if_fail(source.ptr() != NULL);
  
  const gchar* filename = gimp_data_get_filename (GIMP_DATA(source.ptr()));
  
  List settings          = mypaint_brush_get_brush_settings ();
  List switches          = mypaint_brush_get_brush_switch_settings ();
  List texts             = mypaint_brush_get_brush_text_settings ();
  List inputs            = mypaint_brush_get_input_settings ();
  CString name             = g_strdup(source.get("name"));
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(source->p);
  ScopedPointer<FILE, int(FILE*), fclose> f = g_fopen(filename,"w");

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
