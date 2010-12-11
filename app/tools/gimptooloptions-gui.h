/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_TOOL_OPTIONS_GUI_H__
#define __GIMP_TOOL_OPTIONS_GUI_H__

typedef struct _GimpToolOptionsTableIncrement GimpToolOptionsTableIncrement;
typedef void (*GimpPopupCreateViewCallback) (GtkWidget *button, GtkWidget** result, GObject *config);
typedef void (*GimpPopupCreateViewCallbackExt) (GtkWidget *button, GtkWidget** result, GObject *config, gpointer data);

struct _GimpToolOptionsTableIncrement
{
  gint table_col;
  gint table_row;
  gboolean horizontal;
};

GtkWidget * gimp_tool_options_gui        (GimpToolOptions *tool_options);
GtkWidget * gimp_tool_options_gui_full   (GimpToolOptions *tool_options, gboolean horizontal);
GtkWidget * gimp_tool_options_table (gint num_items, gboolean horizontal);
GimpToolOptionsTableIncrement gimp_tool_options_table_increment (gboolean horizontal);
gint gimp_tool_options_table_increment_get_col (GimpToolOptionsTableIncrement* inc);
gint gimp_tool_options_table_increment_get_row (GimpToolOptionsTableIncrement* inc);
void gimp_tool_options_table_increment_next (GimpToolOptionsTableIncrement* inc);

void gimp_tool_options_setup_popup_layout (GList *children, gboolean hide_label);
GtkWidget * gimp_tool_options_frame_gui_with_popup (GObject                    *config,
                                                    GType                       tool_type, 
                                                    gchar                      *label,
                                                    gboolean                    horizontal,
                                                    GimpPopupCreateViewCallback create_view);

GtkWidget * gimp_tool_options_toggle_gui_with_popup (GObject                    *config,
                                                     GType                       tool_type, 
                                                     gchar                      *property_name,
                                                     gchar                      *short_label,
                                                     gchar                      *long_label,
                                                     gboolean                    horizontal,
                                                     GimpPopupCreateViewCallback create_view);

GtkWidget * gimp_tool_options_button_with_popup (GObject                    *config,
                                                 GtkWidget                  *label_widget,
                                                 GimpPopupCreateViewCallbackExt create_view,
                                                 gpointer                    data,
                                                 void (*destroy_data) (gpointer data));

void gimp_tool_options_scale_entry_new  (GObject      *config,
                                          const gchar  *property_name,
                                          GtkTable     *table,
                                          gint          column,
                                          gint          row,
                                          const gchar  *label,
                                          gdouble       step_increment,
                                          gdouble       page_increment,
                                          gint          digits,
                                          gboolean      limit_scale,
                                          gdouble       lower_limit,
                                          gdouble       upper_limit,
                                          gboolean      logarithm,
                                          gboolean      horizontal);

void gimp_tool_options_opacity_entry_new  (GObject      *config,
                                           const gchar   *property_name,
                                           GtkTable      *table,
                                           gint           column,
                                           gint           row,
                                           const gchar   *label,
                                           gboolean      horizontal);

#endif  /*  __GIMP_TOOL_OPTIONS_GUI_H__  */
