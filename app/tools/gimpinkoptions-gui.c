/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "config/gimpconfig-utils.h"

#include "paint/gimpinkoptions.h"

#include "widgets/gimpblobeditor.h"
#include "widgets/gimppropwidgets.h"

#include "gimpinkoptions-gui.h"
#include "gimptooloptions-gui.h"
#include "gimppaintoptions-gui.h"

#include "gimp-intl.h"


static GtkWidget * gimp_ink_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal);
static void sensitivity_create_view (GtkWidget *button, GtkWidget **result, GObject *config);
static void blob_create_view (GtkWidget *button, GtkWidget **result, GObject *config);

GtkWidget *
gimp_ink_options_gui (GimpToolOptions *tool_options)
{
  return gimp_ink_options_gui_full (tool_options, FALSE);
}

GtkWidget *
gimp_ink_options_gui_horizontal (GimpToolOptions *tool_options)
{
  return gimp_ink_options_gui_full (tool_options, TRUE);
}

GtkWidget *
gimp_ink_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal)
{
  GObject        *config      = G_OBJECT (tool_options);
  GtkWidget      *vbox        = gimp_paint_options_gui_full (tool_options, horizontal);
  GtkWidget      *frame;
  GtkWidget      *vbox2;
  GtkWidget      *scale;

  GType          tool_type = G_TYPE_NONE;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (horizontal);

  /* adjust sliders */

  if (horizontal)
    {
      vbox2 = vbox;
    }
  else
    {
      frame = gimp_frame_new (_("Adjustment"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_widget_show (frame);
      vbox2 = gimp_tool_options_gui_full (tool_options, horizontal);
      gtk_container_add (GTK_CONTAINER (frame), vbox2);
      gtk_widget_show (vbox2);
    }

  /*  size slider  */
  scale = gimp_prop_spin_scale_new (config, "size",
                                    _("Size"),
                                    1.0, 2.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* angle adjust slider */
  scale = gimp_prop_spin_scale_new (config, "tilt-angle",
                                    _("Angle"),
                                    1.0, 10.0, 1);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* sens sliders */
  frame = gimp_tool_options_frame_gui_with_popup (config, tool_type,
                                                  _("Sensitivity"),
                                                  horizontal, sensitivity_create_view);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  /*  bottom hbox */

  /* Blob shape widget */
  frame = gimp_tool_options_frame_gui_with_popup (config, tool_type,
                                                  _("Ink Blob"),
                                                  horizontal, blob_create_view);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

  if (horizontal)
    {
      GList *children;
      children = gtk_container_get_children (GTK_CONTAINER (vbox2));  
      gimp_tool_options_setup_popup_layout (children, FALSE);
    }
    
  return vbox;
}

static void
sensitivity_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *vbox2;
  GtkWidget *scale;
  GList *children;

  vbox2 = gtk_vbox_new (FALSE, 2);
  gtk_widget_show (vbox2);

  /* size sens slider */
  scale = gimp_prop_spin_scale_new (config, "size-sensitivity",
                                    _("Size"),
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* tilt sens slider */
  scale = gimp_prop_spin_scale_new (config, "tilt-sensitivity",
                                    _("Tilt"),
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /* velocity sens slider */
  scale = gimp_prop_spin_scale_new (config, "vel-sensitivity",
                                    _("Speed"),
                                    0.01, 0.1, 2);
  gtk_box_pack_start (GTK_BOX (vbox2), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);


  children = gtk_container_get_children (GTK_CONTAINER (vbox2));  
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = vbox2;
}

static void
blob_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GimpInkOptions * ink_options = GIMP_INK_OPTIONS (config);
  GtkWidget *frame;
  GtkWidget *blob_box;
  GtkWidget *hbox;
  GtkWidget *editor;

  /* Blob shape widgets */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_widget_show (hbox);

  /* Blob type radiobuttons */
  blob_box = gimp_prop_enum_stock_box_new (config, "blob-type",
                                           "gimp-shape", 0, 0);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (blob_box),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (hbox), blob_box, FALSE, FALSE, 0);
  gtk_widget_show (blob_box);


  /* Blob editor */
  frame = gtk_frame_new("");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  editor = gimp_blob_editor_new (ink_options->blob_type,
                                 ink_options->blob_aspect,
                                 ink_options->blob_angle);
  gtk_widget_set_size_request (editor, 80, 80);
  gtk_container_add (GTK_CONTAINER (frame), editor);
  gtk_widget_show (editor);

  gimp_config_connect (config, G_OBJECT (editor), NULL);
  
  *result = hbox;
}
