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

extern "C" {
#include "config.h"

#include <glib.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "core/core-types.h"
#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimptoolinfo.h"
#include "core/gimpmypaintbrush.h"
#include "core/mypaintbrush-enum-settings.h"
#include "core/mypaintbrush-brushsettings.h"

#include "gimp-intl.h"
}; // extern "C"

extern "C++" {
#include "core/gimpmypaintbrush-private.hpp"
#include "base/scopeguard.hpp"
#include "base/glib-cxx-utils.hpp"
#include "gimpmypaintoptions-history.hpp"

namespace GimpMypaint {

static OptionsHistory* instance = NULL;
static void free_brush(GimpMypaintBrush* brush);

  
OptionsHistory::OptionsHistory() : brushes(NULL)
{
}

OptionsHistory::~OptionsHistory()
{
  if (brushes)
    g_list_free_full(brushes, GDestroyNotify(free_brush));
}

void
OptionsHistory::push_brush(GimpMypaintBrush* brush)
{
  brushes = g_list_append(brushes, brush);
  g_print("Add custom brush to the history: %s\n", gimp_data_get_filename(GIMP_DATA(brush)) );
  gimp_data_save(GIMP_DATA(brush), NULL);
}

GimpMypaintBrush* 
OptionsHistory::get_brush(int index)
{
  GList* item = g_list_nth(brushes, index);
  return item? GIMP_MYPAINT_BRUSH(item->data) : NULL;
}

int 
OptionsHistory::get_brush_history_size()
{
  return g_list_length(brushes);
}

OptionsHistory* 
OptionsHistory::get_singleton()
{
  if (!instance) {
    instance = new OptionsHistory();
  }
  return instance;
}

static void
free_brush (GimpMypaintBrush* brush)
{
  g_object_unref (brush);
}

}; // namespace GimpMypaint

}; // extern "C++"