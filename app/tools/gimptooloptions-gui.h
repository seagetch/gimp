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

#endif  /*  __GIMP_TOOL_OPTIONS_GUI_H__  */
