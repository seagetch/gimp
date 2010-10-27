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

#include "config.h"

#include <gtk/gtk.h>

#include "tools-types.h"

#include "core/gimptooloptions.h"

#include "gimptooloptions-gui.h"


/*  public functions  */
GtkWidget *
gimp_tool_options_gui (GimpToolOptions *tool_options)
{
  return gimp_tool_options_gui_full (tool_options, FALSE);
}

GtkWidget *
gimp_tool_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal)
{
  GtkWidget *vbox;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  if (horizontal)
    {
      vbox = gtk_hbox_new (FALSE, 6);
    }
  else
    {
      vbox = gtk_vbox_new (FALSE, 6);
    }

  return vbox;
}

GtkWidget * gimp_tool_options_table (gint num_items, gboolean horizontal)
{
  GtkWidget *table;
  
  g_return_val_if_fail (num_items > 0, NULL);
  
  if (horizontal)
    {
      table = gtk_table_new (1, num_items * 3, FALSE);
    }
  else
    {
      table = gtk_table_new (num_items, 3, FALSE);
    }
  
  return table;
}

GimpToolOptionsTableIncrement gimp_tool_options_table_increment (gboolean horizontal)
{
  GimpToolOptionsTableIncrement result;
  result.table_col = result.table_row = 0;
  result.horizontal = horizontal;
  return result;
}

inline 
gint gimp_tool_options_table_increment_get_col (GimpToolOptionsTableIncrement* inc)
{
  return inc->table_col;
}

inline 
gint gimp_tool_options_table_increment_get_row (GimpToolOptionsTableIncrement* inc)
{
  return inc->table_row;
}

inline
void gimp_tool_options_table_increment_next (GimpToolOptionsTableIncrement* inc)
{
  if (inc->horizontal)
    inc->table_col += 3;
  else
    inc->table_row ++;
}

