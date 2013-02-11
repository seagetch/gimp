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

extern "C" {
#include "config.h"

#include <string.h>
#include <math.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/base-types.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"
#include "base/pixel-processor.h"

#include "core/gimp.h"
#include "core/core-types.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"

#include "gimpmypaintcore.hpp"
#include "gimpmypaintcore-surface.hpp"

#include "gimp-intl.h"
};

#include "gimpmypaintcore-postprocess-filters.hpp"

////////////////////////////////////////////////////////////////////////////////
namespace GimpMypaint {
typedef std::vector<Stroke::StrokeRecord> Records;
void
SimulatePressureTask::run (Core* core,
                           GimpDrawable* drawable,
                           Stroke* stroke)
{
  g_print("SimulatePressureTask::run\n");
  Records& records = stroke->get_records();
  gdouble total_length   = 0.;
  for (Records::iterator i = records.begin(); i < records.end(); i ++) {
    if (i->coords.pressure <= 0.0)
      continue;
    total_length += sqrt(i->coords.x * i->coords.x + i->coords.y * i->coords.y);
  }
  gdouble length = 0;
  for (Records::iterator i = records.begin(); i < records.end(); i ++) {
    if (i->coords.pressure <= 0.0)
      continue;
    length += sqrt(i->coords.x * i->coords.x + i->coords.y * i->coords.y);
    gdouble distance = 1.0 - 2 * fabs(length - total_length / 2) / total_length;
    gdouble pressure = sin(M_PI / 2 * distance);
    i->coords.pressure = pressure;
  }
}

void
RenderTask::run (Core* core,
                 GimpDrawable* drawable,
                 Stroke* stroke)
{
  g_print("RenderTask::run\n");
  GimpMypaintSurface* surface = core->get_surface();
  g_return_if_fail(surface != NULL);
  
  stroke->render(surface);
}

}; // namespace GimpMypaint