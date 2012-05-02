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
#include "gimpmypaintbrush-load.h"
#include "gimpmypaintbrush-private.hpp"
#include "mypaintbrush-brushsettings.h"

#include "gimp-intl.h"

}

#define CURRENT_BRUSHFILE_VERSION 2

/*  local function prototypes  */

class MyPaintBrushReader {
  private:
  GimpMypaintBrush *result;
  GHashTable       *raw_pair;
  gint64            version;
  
  typedef gfloat (TransformY)(gfloat input);

  GHashTable* read_file (const gchar *filename, GError **error);
  void load_defaults   ();
  void parse_raw       (gint64 version);
  void parse_points_v1 (gchar *string, gint inputs_index, Mapping *mapping, TransformY trans = NULL);
  void parse_points_v2 (gchar *string, gint inputs_index, Mapping *mapping, TransformY trans = NULL);
  gchar *unquote        (const gchar *string);
  void load_icon       (gchar *filename);
  void dump();
  
  public:
  MyPaintBrushReader();
  ~MyPaintBrushReader();
  GimpMypaintBrush *
  load_brush (GimpContext  *context,
              const gchar  *filename,
              GError      **error);

    
};

extern "C" {
/*  public functions  */

GList *
gimp_mypaint_brush_load (GimpContext  *context,
                 const gchar  *filename,
                 GError      **error)
{
  GimpMypaintBrush *brush;

  g_print ("Read mypaint brush %s\n", filename);

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_path_is_absolute (filename), NULL);
  g_return_val_if_fail (error == NULL || *error == NULL, NULL);
  

  MyPaintBrushReader reader;
  brush = reader.load_brush (context, filename, error);

  if (! brush)
    return NULL;

  return g_list_prepend (NULL, brush);
}

}

/*  private functions  */


MyPaintBrushReader::MyPaintBrushReader() : result(NULL), raw_pair(NULL), version(0)
{
}

MyPaintBrushReader::~MyPaintBrushReader()
{
  if (raw_pair)
    g_hash_table_unref (raw_pair);
  if (result)
    g_object_unref (result);
}

GimpMypaintBrush*
MyPaintBrushReader::load_brush (GimpContext  *context,
                       const gchar  *filename,
                       GError      **error)
{
//  ScopeGuard<gchar, void(gpointer)> unescaped_path(unquote (filename), g_free);
  ScopeGuard<gchar, void(gpointer)> basename(g_path_get_basename (filename), g_free);
  gchar *brushname = g_strndup(basename.ptr(), strlen(basename.ptr()) - strlen(GIMP_MYPAINT_BRUSH_FILE_EXTENSION));
  version = 0;  

  result = GIMP_MYPAINT_BRUSH (gimp_mypaint_brush_new (context, brushname));
  load_defaults();
  g_object_ref (result);
  raw_pair = read_file (filename, error);
  parse_raw (version);
//  dump();
  
  ScopeGuard<gchar, void(gpointer)> filename_dup(g_strndup(filename, strlen(filename) - strlen(GIMP_MYPAINT_BRUSH_FILE_EXTENSION)), g_free);
  ScopeGuard<gchar, void(gpointer)> icon_filename(g_strconcat (filename_dup.ptr(), GIMP_MYPAINT_BRUSH_ICON_FILE_EXTENSION, NULL), g_free);
  g_print ("Read Icon: %s\n", icon_filename.ptr());
  load_icon (icon_filename.ptr());
  return result;
}

void
MyPaintBrushReader::load_defaults ()
{
  ScopeGuard<GList, void(GList*)> settings(mypaint_brush_get_brush_settings (), g_list_free);
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(result->p);

  for (GList* item = settings.ptr(); item; item = item->next) {
    MyPaintBrushSettings* setting = reinterpret_cast<MyPaintBrushSettings*>(item->data);
    priv->set_base_value(setting->index, setting->default_value);
//    priv->deallocate_mapping(setting->index);
    if (g_strcmp0(setting->internal_name, "opaque_multiply") == 0) {
      priv->allocate_mapping(setting->index);
      GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
      v->mapping->set_n(INPUT_PRESSURE, 2);
      v->mapping->set_point(INPUT_PRESSURE, 0, 0.0, 0.0);
      v->mapping->set_point(INPUT_PRESSURE, 1, 1.0, 1.0);      
    }
  }
}

void
MyPaintBrushReader::dump ()
{
  ScopeGuard<GList, void(GList*)> settings(mypaint_brush_get_brush_settings (), g_list_free);
  ScopeGuard<GList, void(GList*)> inputs(mypaint_brush_get_input_settings (), g_list_free);
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(result->p);

  for (GList* item = settings.ptr(); item; item = item->next) {
    MyPaintBrushSettings* setting = reinterpret_cast<MyPaintBrushSettings*>(item->data);
    GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
    if (v->mapping) {
      g_print("mapping: %s=%f\n", setting->internal_name, v->mapping->base_value);
      for (GList* input_iter = inputs.ptr(); input_iter; input_iter = input_iter->next) {
        MyPaintBrushInputSettings* input = reinterpret_cast<MyPaintBrushInputSettings*>(input_iter->data);
        int n = v->mapping->get_n(input->index);
        if (n == 0)
          continue;
        g_print ("  %s:", input->name);
        for (int i = 0; i < n; i ++) {
          float x, y;
          v->mapping->get_point(input->index, i, &x, &y);
          g_print ("(%f,%f) ", x, y);
        }
        g_print ("\n");
      }
    } else {
      g_print("base only: %s=%f\n", setting->internal_name, v->base_value);
    }
  }
}

GHashTable*
MyPaintBrushReader::read_file (const gchar *filename, GError **error)
{
  GIOChannel       *config_stream;
  gchar            *line;
  gsize             line_len;
  gsize             line_term_pos;

  g_return_val_if_fail ( GIMP_IS_MYPAINT_BRUSH (result), NULL);

  config_stream = g_io_channel_new_file (filename, "r", error);
  ScopeGuard<GIOChannel, void(GIOChannel*)> config_stream_holder(config_stream, g_io_channel_unref);
    
  if (! config_stream)
    {
      g_clear_error (error);
      return NULL;
    }
    
  raw_pair = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  
  while (g_io_channel_read_line (config_stream, &line, &line_len, &line_term_pos, error) != G_IO_STATUS_EOF)
    {
      ScopeGuard<gchar, void(gpointer)> line_holder(line, g_free);
      if (!line)
        break;
      
      line = g_strstrip (line);
      if (line[0] != '#')
        {
          gchar **tokens = g_strsplit (line, " ", 2);
          ScopeGuard<gchar*, void(gchar**)> token_holder(tokens, g_strfreev);
          if (g_strcmp0(tokens[0], "version") == 0) 
            {
              version = g_ascii_strtoll (tokens[1], NULL, 0);
              if (version > CURRENT_BRUSHFILE_VERSION)
                {
                  // TBD: 
                  break;
                }
            } 
          else
            {
              g_hash_table_insert (raw_pair, g_strdup(tokens[0]), g_strdup(tokens[1])); // Note: ownership of tokens is moved to GHashTable object.
            }
        }
    }
  return raw_pair;
}

gchar* 
MyPaintBrushReader::unquote (const gchar *string)
{
  return g_uri_unescape_string (string, NULL);
}

/// Parses the points list format from versions prior to version 2.
void
MyPaintBrushReader::parse_points_v1 (
    gchar *string, 
    gint inputs_index, 
    Mapping *mapping, 
    MyPaintBrushReader::TransformY* trans)
{
  gchar **iter;
  gint num_seq = 0;
  gfloat prev_x;
  gint i;
  
  g_print ("Parse v1 params : %s\n", string);

  ScopeGuard<gchar*, void(gchar**)> seq(g_strsplit (string, " ", 0), g_strfreev); 
  
  for (iter = seq.ptr(); *iter; iter++)
    num_seq ++;

  mapping->set_n(inputs_index, num_seq / 2);
  g_print ("set_n: %d\n", num_seq / 2);

  prev_x = 0;
  i = 0;
  for (iter = seq.ptr(); *iter; )
    {
      gdouble x_val, y_val;
      
      x_val = g_ascii_strtod (*(iter++), NULL);
      if (!(x_val > prev_x));
        {
          g_print ("Invalid input points: x > previous x\n");
          goto next;
        }
      prev_x = x_val;

      y_val = g_ascii_strtod (*(iter++), NULL);
      if (trans)
        y_val = (*trans)((gfloat)y_val);
      
      g_print ("X=%lf,Y=%lf\n", x_val, y_val);
      
      mapping->set_point(inputs_index, i ++, (float)x_val, (float)y_val);
    next:;;
    }  
}

/// Parses the newer points list format of v2 and beyond.
void
MyPaintBrushReader::parse_points_v2 (
    gchar *string, 
    gint inputs_index, 
    Mapping *mapping,
    MyPaintBrushReader::TransformY* trans)

{
  gchar **seq;
  gchar **iter;
  gint num_seq = 0;
  gint i;

  seq = g_strsplit (string, ", ", 0);  
  
  for (iter = seq; *iter; iter++)
    num_seq ++;

  mapping->set_n(inputs_index, num_seq);

  i = 0;
  for (iter = seq; *iter; iter++)
    {
      gchar *str_pos = *iter;
      gdouble x_val, y_val;
      str_pos = g_strstrip (str_pos);
      if (str_pos[0] != '(') 
        {
          // NG
          g_print ("ERROR: must be start with '(', but %c appears.\n", str_pos[0]);
        }
      str_pos = g_strstrip (++str_pos);
      x_val = g_ascii_strtod (str_pos, &str_pos);
      
      str_pos = g_strstrip (str_pos);
      y_val = g_ascii_strtod (str_pos, &str_pos);
      if (trans)
        y_val = (*trans)((gfloat)y_val);

      str_pos = g_strstrip (str_pos);
      if (str_pos[0] != ')') 
        {
          // NG
          g_print ("ERROR: must be end with ')', but %c appears.\n", str_pos[0]);
        }
      mapping->set_point(inputs_index, i ++, (float)x_val, (float)y_val);
    }  
}

void
MyPaintBrushReader::parse_raw (
                  gint64            version)
{
  ScopeGuard<GHashTable, void(GHashTable*)> settings_dict(mypaint_brush_get_brush_settings_dict (), g_hash_table_unref);
  ScopeGuard<GHashTable, void(GHashTable*)> inputs_dict(mypaint_brush_get_input_settings_dict (), g_hash_table_unref);
  ScopeGuard<GHashTable, void(GHashTable*)> migrate_dict(mypaint_brush_get_setting_migrate_dict (), g_hash_table_unref);
  GHashTableIter    raw_pair_iter;
  gpointer          raw_key, raw_value;
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(result->p);

  g_hash_table_iter_init (&raw_pair_iter, raw_pair);
  
  while (g_hash_table_iter_next (&raw_pair_iter, &raw_key, &raw_value))
    {
      gchar    *key          = (gchar*)raw_key;
      gchar    *value        = (gchar*)raw_value;
      
      if (strcmp (key, "parent_brush_name") == 0)
        {
          gchar *uq_value = unquote (value);
          priv->set_parent_brush_name(uq_value);
          g_object_set (G_OBJECT (result), "name", uq_value, NULL);
//          g_free (uq_value);
          goto next_pair;
        }
      else if (strcmp (key, "group") == 0)
        {
          gchar *uq_value = unquote (value);
          priv->set_group(value);
          g_free (uq_value);
          goto next_pair;
        }
      else if (version <= 1 && strcmp (key, "color") == 0) 
        {
          ScopeGuard<gchar*, void(gchar**)>  tokens(g_strsplit(value, " ", 0), g_strfreev);
          MyPaintBrushSettings               *setting;
          gint64 r, g, b;
          r = g_ascii_strtoll(tokens.ptr()[0], NULL, 10);
          g = g_ascii_strtoll(tokens.ptr()[1], NULL, 10);
          b = g_ascii_strtoll(tokens.ptr()[2], NULL, 10);
          GimpRGB rgb;
          rgb.r = r / 255.0; 
          rgb.g = g / 255.0; 
          rgb.b = b / 255.0; 
          rgb.a = 1.0;
          GimpHSV hsv;
          gimp_rgb_to_hsv (&rgb, &hsv);

          setting = (MyPaintBrushSettings*)g_hash_table_lookup (settings_dict.ptr(), "color_h");
          priv->set_base_value (setting->index, hsv.h);

          setting = (MyPaintBrushSettings*)g_hash_table_lookup (settings_dict.ptr(), "color_s");
          priv->set_base_value (setting->index, hsv.s);

          setting = (MyPaintBrushSettings*)g_hash_table_lookup (settings_dict.ptr(), "color_v");
          priv->set_base_value (setting->index, hsv.v);

          goto next_pair;
        }
      else if (version <= 1 && strcmp (key, "change_radius") == 0)
        {
          if (strcmp (value, "0.0") == 0)
            goto next_pair;
          else
            {
              //FIXME! ERROR
              //raise Obsolete, 'change_radius is not supported any more'
            }
        }
      else if (version <= 2 && strcmp (key, "adapt_color_from_image") == 0)
        {
          if (strcmp (value, "0.0") == 0)
            goto next_pair;
          else
            {
              //FIXME! ERROR
              //raise Obsolete, 'adapt_color_from_image is obsolete, ignored;' + \
              //                ' use smudge and smudge_length instead'
            }
        }
      else if (version <= 1 && strcmp (key, "painting_time") == 0)
        {
          goto next_pair;
        }      
      else if (version <= 1 && strcmp (key, "speed") == 0)
        {
          key = (gchar*)"speed1";
        }
      {
      MyPaintBrushSettingMigrate *migrate = (MyPaintBrushSettingMigrate*)g_hash_table_lookup (migrate_dict.ptr(), key);
      if (migrate)
        {
          if (migrate->new_name)
            key = migrate->new_name;
        }
      
      
      MyPaintBrushSettings             *setting;
      setting = (MyPaintBrushSettings*)g_hash_table_lookup (settings_dict.ptr(), key);
      if (setting)
        {
          ScopeGuard<gchar*, void(gchar**)>   split_values(g_strsplit (value, "|", 0), g_strfreev);
          gchar                          **values_pos;
          values_pos = split_values.ptr();
          
          gdouble d_val = g_ascii_strtod (*values_pos, NULL);
          if (migrate && migrate->transform)
            d_val = (*migrate->transform)((gfloat)d_val);

        
          priv->set_base_value(setting->index, d_val);
          values_pos ++;      
          if (*values_pos)
            {
              priv->allocate_mapping(setting->index);
            }
          GimpMypaintBrushPrivate::Value* v = priv->get_setting(setting->index);
          while (*values_pos)
            {
              //      inputname, rawpoints = part.strip().split(' ', 1)
              gchar *str_pos = *values_pos;
              str_pos = g_strstrip (str_pos);
              ScopeGuard<gchar *, void(gchar**)> tokens(g_strsplit (str_pos, " ", 2), g_strfreev);
              MyPaintBrushInputSettings *input_setting;
              input_setting = (MyPaintBrushInputSettings*)g_hash_table_lookup (inputs_dict.ptr(), tokens.ptr()[0]);
              if (tokens.ptr()[1] && input_setting)
                {
                  if (version <= 1)
                    parse_points_v1 (tokens.ptr()[1], input_setting->index, v->mapping, migrate? migrate->transform: NULL);
                  else
                    parse_points_v2 (tokens.ptr()[1], input_setting->index, v->mapping, migrate? migrate->transform: NULL);
                }
              else
                {
                  g_print ("invalid parameter '%s'\n", *values_pos);
                }
              values_pos ++;
            }
        }
      else
        {
          g_print ("invalid key '%s'\n", key);
        }
      }
    next_pair:
      ;;
    }
}

void
MyPaintBrushReader::load_icon (gchar *filename)
{
  result->icon = cairo_image_surface_create_from_png(filename);
}
