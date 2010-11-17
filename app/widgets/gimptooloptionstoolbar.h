/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimptooloptionstoolbar.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_TOOL_OPTIONS_TOOLBAR_H__
#define __GIMP_TOOL_OPTIONS_TOOLBAR_H__


#include <gtk/gtk.h>
#include "core/gimptooloptions.h"

#define GIMP_TYPE_TOOL_OPTIONS_TOOLBAR            (gimp_tool_options_toolbar_get_type ())
#define GIMP_TOOL_OPTIONS_TOOLBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_TOOL_OPTIONS_TOOLBAR, GimpToolOptionsToolbar))
#define GIMP_TOOL_OPTIONS_TOOLBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_TOOL_OPTIONS_TOOLBAR, GimpToolOptionsToolbarClass))
#define GIMP_IS_TOOL_OPTIONS_TOOLBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_TOOL_OPTIONS_TOOLBAR))
#define GIMP_IS_TOOL_OPTIONS_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_TOOL_OPTIONS_TOOLBAR))
#define GIMP_TOOL_OPTIONS_TOOLBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_TOOL_OPTIONS_TOOLBAR, GimpToolOptionsToolbarClass))

typedef struct _GimpToolOptionsToolbar         GimpToolOptionsToolbar;
typedef struct _GimpToolOptionsToolbarPrivate  GimpToolOptionsToolbarPrivate;
typedef struct _GimpToolOptionsToolbarClass    GimpToolOptionsToolbarClass;

struct _GimpToolOptionsToolbar
{
  GtkToolbar                    parent_instance;

  GimpToolOptionsToolbarPrivate *p;
};

struct _GimpToolOptionsToolbarClass
{
  GtkToolbarClass  parent_class;
};


GType             gimp_tool_options_toolbar_get_type         (void) G_GNUC_CONST;
GtkWidget       * gimp_tool_options_toolbar_new              (Gimp                  *gimp,
                                                             GimpMenuFactory       *menu_factory);
GimpToolOptions * gimp_tool_options_toolbar_get_tool_options (GimpToolOptionsToolbar *toolbar);


#endif  /*  __GIMP_TOOL_OPTIONS_TOOLBAR_H__  */
