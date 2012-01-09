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

#include "actions-types.h"

#include "core/gimpmypaintbrush.h"
#include "core/gimpcontext.h"

#include "widgets/gimpactiongroup.h"
#include "widgets/gimphelp-ids.h"

#include "actions.h"
#include "mypaint-brushes-actions.h"
#include "data-commands.h"

#include "gimp-intl.h"


static const GimpActionEntry mypaint_brushes_actions[] =
{
  { "mypaint-brushes-popup", GIMP_STOCK_BRUSH,
    NC_("mypaint-brushes-action", "Mypaint Brushes Menu"), NULL, NULL, NULL,
    GIMP_HELP_MYPAINT_BRUSH_DIALOG },
#if 0
  { "brushes-open-as-image", GTK_STOCK_OPEN,
    NC_("brushes-action", "_Open Brush as Image"), "",
    NC_("brushes-action", "Open brush as image"),
    G_CALLBACK (data_open_as_image_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_OPEN_AS_IMAGE },

  { "brushes-new", GTK_STOCK_NEW,
    NC_("brushes-action", "_New Brush"), "",
    NC_("brushes-action", "Create a new brush"),
    G_CALLBACK (data_new_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_NEW },

  { "brushes-duplicate", GIMP_STOCK_DUPLICATE,
    NC_("brushes-action", "D_uplicate Brush"), NULL,
    NC_("brushes-action", "Duplicate this brush"),
    G_CALLBACK (data_duplicate_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_DUPLICATE },
#endif
  { "mypaint-brushes-copy-location", GTK_STOCK_COPY,
    NC_("mypaint-brushes-action", "Copy Mypaint Brush _Location"), "",
    NC_("mypaint-brushes-action", "Copy mypaint brush file location to clipboard"),
    G_CALLBACK (data_copy_location_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_COPY_LOCATION },

  { "mypaint-brushes-delete", GTK_STOCK_DELETE,
    NC_("mypaint-brushes-action", "_Delete Mypaint Brush"), "",
    NC_("mypaint-brushes-action", "Delete this mypaint brush"),
    G_CALLBACK (data_delete_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_DELETE },

  { "mypaint-brushes-refresh", GTK_STOCK_REFRESH,
    NC_("mypaint-brushes-action", "_Refresh Mypaint Brushes"), "",
    NC_("mypaint-brushes-action", "Refresh Mypaint brushes"),
    G_CALLBACK (data_refresh_cmd_callback),
    GIMP_HELP_MYPAINT_BRUSH_REFRESH }
};

#if 0
static const GimpStringActionEntry mypaint_brushes_edit_actions[] =
{
  { "mypaint-brushes-edit", GTK_STOCK_EDIT,
    NC_("mypaint-brushes-action", "_Edit Brush..."), NULL,
    NC_("brushes-action", "Edit this brush"),
    "gimp-brush-editor",
    GIMP_HELP_MYPAINT_BRUSH_EDIT }
};
#endif


void
mypaint_brushes_actions_setup (GimpActionGroup *group)
{
  gimp_action_group_add_actions (group, "mypaint-brushes-action",
                                 mypaint_brushes_actions,
                                 G_N_ELEMENTS (mypaint_brushes_actions));
#if 0
  gimp_action_group_add_string_actions (group, "mypaint-brushes-action",
                                        mypaint_brushes_edit_actions,
                                        G_N_ELEMENTS (brushes_edit_actions),
                                        G_CALLBACK (data_edit_cmd_callback));
#endif
}

void
mypaint_brushes_actions_update (GimpActionGroup *group,
                        gpointer         user_data)
{
  GimpContext *context  = action_data_get_context (user_data);
  GimpMypaintBrush   *mypaint_brush    = NULL;
  GimpData    *data     = NULL;
  const gchar *filename = NULL;

  if (context)
    {
      mypaint_brush = gimp_context_get_mypaint_brush (context);

      if (action_data_sel_count (user_data) > 1)
        {
          mypaint_brush = NULL;
        }

      if (mypaint_brush)
        {
          data = GIMP_DATA (mypaint_brush);

          filename = gimp_data_get_filename (data);
        }
    }

#define SET_SENSITIVE(action,condition) \
        gimp_action_group_set_action_sensitive (group, action, (condition) != 0)
#if 0
  SET_SENSITIVE ("brushes-edit",          brush);
  SET_SENSITIVE ("brushes-duplicate",     brush && GIMP_DATA_GET_CLASS (data)->duplicate);
#endif
  SET_SENSITIVE ("mypaint-brushes-copy-location", mypaint_brush && filename);
  SET_SENSITIVE ("mypaint-brushes-delete",        mypaint_brush && gimp_data_is_deletable (data));

#undef SET_SENSITIVE
}
