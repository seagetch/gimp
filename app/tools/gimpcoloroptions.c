/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-2001 Spencer Kimball, Peter Mattis, and others
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpconfig/gimpconfig.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "widgets/gimppropwidgets.h"

#include "gimphistogramoptions.h"
#include "gimpcoloroptions.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_SAMPLE_MERGED,
  PROP_SAMPLE_AVERAGE,
  PROP_AVERAGE_RADIUS
};


static void   gimp_color_options_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec);
static void   gimp_color_options_get_property (GObject      *object,
                                               guint         property_id,
                                               GValue       *value,
                                               GParamSpec   *pspec);
static void   gimp_color_options_average_create_view (GtkWidget *source, 
                                                       GtkWidget **result, 
                                                       GObject *config);


G_DEFINE_TYPE (GimpColorOptions, gimp_color_options,
               GIMP_TYPE_IMAGE_MAP_OPTIONS)


static void
gimp_color_options_class_init (GimpColorOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = gimp_color_options_set_property;
  object_class->get_property = gimp_color_options_get_property;

  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_MERGED,
                                    "sample-merged", NULL,
                                    FALSE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_BOOLEAN (object_class, PROP_SAMPLE_AVERAGE,
                                    "sample-average", NULL,
                                    TRUE,
                                    GIMP_PARAM_STATIC_STRINGS);
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_AVERAGE_RADIUS,
                                   "average-radius",
                                   _("Color Picker Average Radius"),
                                   1.0, 300.0, 3.0,
                                   GIMP_PARAM_STATIC_STRINGS);
}

static void
gimp_color_options_init (GimpColorOptions *options)
{
}

static void
gimp_color_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpColorOptions *options = GIMP_COLOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      options->sample_merged = g_value_get_boolean (value);
      break;
    case PROP_SAMPLE_AVERAGE:
      options->sample_average = g_value_get_boolean (value);
      break;
    case PROP_AVERAGE_RADIUS:
      options->average_radius = g_value_get_double (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_color_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpColorOptions *options = GIMP_COLOR_OPTIONS (object);

  switch (property_id)
    {
    case PROP_SAMPLE_MERGED:
      g_value_set_boolean (value, options->sample_merged);
      break;
    case PROP_SAMPLE_AVERAGE:
      g_value_set_boolean (value, options->sample_average);
      break;
    case PROP_AVERAGE_RADIUS:
      g_value_set_double (value, options->average_radius);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

GtkWidget *
gimp_color_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal)
{
  GObject   *config = G_OBJECT (tool_options);
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *scale;
 
  if (GIMP_IS_HISTOGRAM_OPTIONS (tool_options))
    vbox = gimp_histogram_options_gui (tool_options);
  else
    vbox = gimp_tool_options_gui_full (tool_options, horizontal);

  /*  the sample average options  */
#if 0
  frame = gimp_frame_new (NULL);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  scale = gimp_prop_spin_scale_new (config, "average-radius",
                                    _("Radius"),
                                    1.0, 10.0, 0);
  gtk_container_add (GTK_CONTAINER (frame), scale);
  gtk_widget_show (scale);

  button = gimp_prop_check_button_new (config, "sample-average",
                                       _("Sample average"));
  gtk_frame_set_label_widget (GTK_FRAME (frame), button);
  gtk_widget_show (button);

  // gimp-painter-2.7
  gtk_widget_set_sensitive (scale,
                            GIMP_COLOR_OPTIONS (config)->sample_average);
#endif
  frame = gimp_tool_options_toggle_gui_with_popup (config, G_TYPE_NONE,
                             "sample-average", _("Average"), _("Sample Average"),
                             horizontal, gimp_color_options_average_create_view);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);  

//  g_object_set_data (G_OBJECT (button), "set_sensitive", scale);
#if 0
  g_object_bind_property (config, "sample-average",
                          scale,  "sensitive",
                          G_BINDING_SYNC_CREATE);
#endif

  return vbox;
}


GtkWidget *
gimp_color_options_gui (GimpToolOptions *tool_options)
{
  return gimp_color_options_gui_full (tool_options, FALSE);
}

GtkWidget *
gimp_color_options_gui_horizontal (GimpToolOptions *tool_options)
{
  return gimp_color_options_gui_full (tool_options, TRUE);
}

  
static void
gimp_color_options_average_create_view (GtkWidget *source, GtkWidget **result, GObject *config)
{
  GimpToolOptions *tool_options = GIMP_TOOL_OPTIONS (config);
  GtkWidget       *vbox         = gimp_tool_options_gui (tool_options);
  GtkWidget       *scale;
  GList *children;
  scale = gimp_prop_spin_scale_new (config, "average-radius",
                                    _("Radius"),
                                    1.0, 10.0, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  children = gtk_container_get_children (GTK_CONTAINER (vbox));  
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = vbox;
}
