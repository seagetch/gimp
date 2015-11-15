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

#ifndef __GIMP_IMAGE_WINDOW_PRIVATE_H__
#define __GIMP_IMAGE_WINDOW_PRIVATE_H__

typedef struct _GimpImageWindowPrivate GimpImageWindowPrivate;

struct _GimpImageWindowPrivate
{
  Gimp              *gimp;
  GimpUIManager     *menubar_manager;
  GimpDialogFactory *dialog_factory;
  gboolean			 toolbar_window;

  GList             *shells;
  GimpDisplayShell  *active_shell;

  GtkWidget         *main_vbox;
  GtkWidget         *menubar;
  GtkWidget         *hbox;
  GtkWidget         *left_hpane;
  GtkWidget         *left_docks;
  GtkWidget         *right_hpane;
  GtkWidget         *notebook;
  GtkWidget         *right_docks;
  GtkWidget         *toolbar; /* gimp-painter-2.7 */

  GdkWindowState     window_state;

  const gchar       *entry_id;
};

#define GIMP_IMAGE_WINDOW_GET_PRIVATE(window) \
        G_TYPE_INSTANCE_GET_PRIVATE (window, \
                                     GIMP_TYPE_IMAGE_WINDOW, \
                                     GimpImageWindowPrivate)

#endif