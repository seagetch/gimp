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


static GtkWidget * blob_image_new (GimpInkBlobType blob_type);
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

  GtkWidget      *table;
  GType          tool_type = G_TYPE_NONE;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (horizontal);

  /* adjust sliders */
  table = gimp_tool_options_table (2, horizontal);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);

  if (horizontal)
    {
      gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, TRUE, 0);
    }
  else
    {
      frame = gimp_frame_new (_("Adjustment"));
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
      gtk_widget_show (frame);
      gtk_container_add (GTK_CONTAINER (frame), table);
    }

  gtk_widget_show (table);

  /*  size slider  */
  gimp_tool_options_scale_entry_new (config, "size",
                                     GTK_TABLE (table),
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Size:"),
                                     1.0, 2.0, 1,
                                     FALSE, 0.0, 0.0, TRUE, horizontal);
    
  gimp_tool_options_table_increment_next (&inc);

  /* angle adjust slider */
  gimp_tool_options_scale_entry_new (config, "tilt-angle",
                                     GTK_TABLE (table),
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Angle:"),
                                     1.0, 10.0, 1,
                                     FALSE, 0.0, 0.0, FALSE, horizontal);

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

  return vbox;
}

static GtkWidget *
blob_image_new (GimpInkBlobType blob_type)
{
  const gchar *stock_id = NULL;

  switch (blob_type)
    {
    case GIMP_INK_BLOB_TYPE_CIRCLE:
      stock_id = GIMP_STOCK_SHAPE_CIRCLE;
      break;

    case GIMP_INK_BLOB_TYPE_SQUARE:
      stock_id = GIMP_STOCK_SHAPE_SQUARE;
      break;

    case GIMP_INK_BLOB_TYPE_DIAMOND:
      stock_id = GIMP_STOCK_SHAPE_DIAMOND;
      break;
    }

  return gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
}

static void
sensitivity_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (FALSE);
  GList *children;

  table = gimp_tool_options_table (3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_widget_show (table);

  /* size sens slider */
  gimp_prop_scale_entry_new (config, "size-sensitivity",
                             GTK_TABLE (table),
                             gimp_tool_options_table_increment_get_col (&inc),
                             gimp_tool_options_table_increment_get_row (&inc),
                             _("Size:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);
  gimp_tool_options_table_increment_next (&inc);

  /* tilt sens slider */
  gimp_prop_scale_entry_new (config, "tilt-sensitivity",
                             GTK_TABLE (table),
                             gimp_tool_options_table_increment_get_col (&inc),
                             gimp_tool_options_table_increment_get_row (&inc),
                             _("Tilt:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);
  gimp_tool_options_table_increment_next (&inc);

  /* velocity sens slider */
  gimp_prop_scale_entry_new (config, "vel-sensitivity",
                             GTK_TABLE (table),
                             gimp_tool_options_table_increment_get_col (&inc),
                             gimp_tool_options_table_increment_get_row (&inc),
                             _("Speed:"),
                             0.01, 0.1, 1,
                             FALSE, 0.0, 0.0);
  gimp_tool_options_table_increment_next (&inc);
  
  children = gtk_container_get_children (GTK_CONTAINER (table));
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = table;
}

static void
blob_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkHBox   *hbox;
  GtkWidget *blob_vbox;
  GtkWidget *frame;
  GtkWidget *editor;
  GtkWidget *size_group;
  GtkWidget *blob_box;
  GimpInkOptions * ink_options = GIMP_INK_OPTIONS (config);

  hbox = GTK_HBOX (gtk_hbox_new (FALSE, 2));
  gtk_widget_show (GTK_WIDGET (hbox));

  /* Blob type radiobuttons */
  frame = gimp_prop_enum_radio_frame_new (config, "blob-type",
                                          _("Type"), 0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  hbox = GTK_HBOX (gtk_hbox_new (FALSE, 2));
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  gtk_widget_show (GTK_WIDGET (hbox));

  size_group = GTK_WIDGET (gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL));

#if 1
  /* Blob type radiobuttons */
  blob_box = gimp_prop_enum_stock_box_new (config, "blob-type",
                                           "gimp-shape", 0, 0);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (blob_box),
                                  GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start (GTK_BOX (hbox), blob_box, FALSE, FALSE, 0);
  gtk_widget_show (blob_box);
#else
  frame = gimp_frame_new (_("Shape"));
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);
#endif

  gtk_size_group_add_widget (size_group, blob_box);

  /* Blob editor */
  frame = gtk_aspect_frame_new (NULL, 0.0, 0.5, 1.0, FALSE);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  gtk_size_group_add_widget (size_group, frame);

  editor = gimp_blob_editor_new (ink_options->blob_type,
                                 ink_options->blob_aspect,
                                 ink_options->blob_angle);
  gtk_container_add (GTK_CONTAINER (frame), editor);
  gtk_widget_show (editor);

  gimp_config_connect (config, G_OBJECT (editor), NULL);
  
  *result = GTK_WIDGET (hbox);
}
