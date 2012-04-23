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

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "paint-funcs/paint-funcs.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"
#include "core/gimpmypaintbrush.h"

#include "gimpmypaintcoreundo.h"
#include "gimpmypaintoptions.h"

#include "gimpairbrush.h"

#include "gimp-intl.h"
};

#include "gimpmypaintcore.hpp"
#include "gimpmypaintcore-surface.hpp"
#include "core/gimpmypaintbrush-private.hpp"

/*  local function prototypes  */

GimpMypaintCore::GimpMypaintCore()
{
  ID               = 0; //global_core_ID++;

  surface = NULL;
  brush   = new Brush();
  stroke  = NULL;
  undo_desc = NULL;
}


GimpMypaintCore::~GimpMypaintCore ()
{
  cleanup ();
  if (undo_desc)
    g_free (undo_desc);
  undo_desc = NULL;
}

void GimpMypaintCore::cleanup()
{
  g_print("GimpMypaintCore::cleanup\n");
  if (surface && stroke) {
    split_stroke();
  }
  if (stroke) {
    delete stroke;
    stroke = NULL;
  }
  if (brush) {
    delete brush;
    brush = NULL;
  }
  if (surface) {
    delete surface;
    surface = NULL;
  }
}

void GimpMypaintCore::stroke_to (GimpDrawable* drawable, 
                                 gdouble dtime, 
                                 const GimpCoords* coords,
                                 GimpMypaintOptions* options)
{
//  g_print("entering GimpMypaintCore::stroke_to...\n");
  bool split = false;
  // Prepare Brush.
//  g_print("updating resource...\n");
  update_resource(options);
  
  /// from Document#stroke_to

  // Prepare Stroke object
  if (!stroke) {
//    g_print("create new stroke...\n");
    stroke = new Stroke();
//    g_print("Stroke::start...\n");
    stroke->start(brush);
    // Prepare Surface object for drawable
//    g_print("testing surface...\n");
    if (!surface) {
      g_print("create new surface...\n");
      surface = new GimpMypaintSurface(drawable);
    } else if (!surface->is_surface_for(drawable)) {
      surface->end_session();
      g_print("delete surface...\n");
      delete surface;
      g_print("recreate new surface...\n");
      surface = new GimpMypaintSurface(drawable);
    }
//    g_print("Stroke::begin_session...\n");
    surface->begin_session();
  }
  
//  g_print("Stroke::record...\n");
  stroke->record(dtime, coords);

  /// from Layer#stroke_to
  {
//    g_print("calling Brush::stroke_to(surf,%lf,%lf,%lf,%lf,%lf,%lf)...\n", coords->x, coords->y, coords->pressure, coords->xtilt, coords->ytilt, dtime);
    //surface->begin_atomic();
    split = brush->stroke_to(surface, coords->x, coords->y, 
                             coords->pressure, 
                             coords->xtilt, coords->ytilt, dtime);
    //surface->end_atomic();
  }
  
  if (split) {
    split_stroke();
  }
//  g_print("leaving GimpMypaintCore::stroke_to...\n");
}

void GimpMypaintCore::split_stroke()
{
//  g_print("splitting stroke...\n");
  if (!stroke)
    return;
    
  stroke->stop();
  surface->end_session();
  // push stroke to undo stack.

  delete stroke;  
  stroke = NULL;
  
  if (brush) {
    delete brush;
    brush = NULL;
  }
  return;
}

void GimpMypaintCore::update_resource(GimpMypaintOptions* options)
{
  GimpContext* context = GIMP_CONTEXT (options);
  GimpMypaintBrush* myb = context->mypaint_brush;
  GimpMypaintBrushPrivate *myb_priv = NULL;
  
  if (myb)
    myb_priv = reinterpret_cast<GimpMypaintBrushPrivate*>(myb->p);

  if (mypaint_brush != myb) {
    delete brush;
    brush = NULL;
  }
  if (!brush) {
    brush = new Brush();
    mypaint_brush = myb;
    
    // copy brush setting here.
    if (myb) {
      for (int i = 0; i < BRUSH_SETTINGS_COUNT; i ++) {
        Mapping* m = myb_priv->get_setting(i)->mapping;
        if (m)
          brush->copy_mapping(i, m);
        else {
          brush->set_mapping_n(i, 0, 0);
          brush->set_base_value(i, myb_priv->get_setting(i)->base_value);
        }
      }
    }
    
    // update color values
    GimpRGB rgb;
    gimp_context_get_foreground(context, &rgb);
    GimpHSV hsv;
    gimp_rgb_to_hsv (&rgb, &hsv);
    brush->set_base_value(BRUSH_COLOR_H, hsv.h);
    brush->set_base_value(BRUSH_COLOR_S, hsv.s);
    brush->set_base_value(BRUSH_COLOR_V, hsv.v);
    
    brush->reset();
  }
  
}

void GimpMypaintCore::reset_brush()
{
  if (brush) {
    brush->reset();
  }
}

void GimpMypaintCore::set_undo_desc(gchar* value)
{
//  g_free (undo_desc);
//  undo_desc = g_strdup (value);
}


#if 0
/*  public functions  */

void
gimp_mypaint_core_paint (GimpMypaintCore    *core,
                       GimpDrawable     *drawable,
                       GimpMypaintOptions *mypaint_options,
                       GimpMypaintState    mypaint_state,
                       guint32           time)
{
  GimpMypaintCoreClass *core_class;

  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_MYPAINT_OPTIONS (mypaint_options));

  core_class = GIMP_MYPAINT_CORE_GET_CLASS (core);

  if (core_class->pre_paint (core, drawable,
                             mypaint_options,
                             mypaint_state, time))
    {

      if (mypaint_state == GIMP_MYPAINT_STATE_MOTION)
        {
          /* Save coordinates for gimp_mypaint_core_interpolate() */
          core->last_mypaint.x = core->cur_coords.x;
          core->last_mypaint.y = core->cur_coords.y;
        }

      core_class->maint (core, drawable,
                         mypaint_options,
                         &core->cur_coords,
                         mypaint_state, time);

      core_class->post_paint (core, drawable,
                              mypaint_options,
                              mypaint_state, time);
    }
}
#endif

#if 0
void
gimp_mypaint_core_cancel (GimpMypaintCore *core,
                        GimpDrawable  *drawable)
{
  gint x, y;
  gint width, height;

  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if ((core->x2 == core->x1) || (core->y2 == core->y1))
    return;

  if (gimp_rectangle_intersect (core->x1, core->y1,
                                core->x2 - core->x1,
                                core->y2 - core->y1,
                                0, 0,
                                gimp_item_get_width  (GIMP_ITEM (drawable)),
                                gimp_item_get_height (GIMP_ITEM (drawable)),
                                &x, &y, &width, &height))
    {
      gimp_mypaint_core_copy_valid_tiles (core->undo_tiles,
                                        gimp_drawable_get_tiles (drawable),
                                        x, y, width, height);
    }

  tile_manager_unref (core->undo_tiles);
  core->undo_tiles = NULL;

  if (core->saved_proj_tiles)
    {
      tile_manager_unref (core->saved_proj_tiles);
      core->saved_proj_tiles = NULL;
    }

  gimp_drawable_update (drawable, x, y, width, height);

  gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
}

void
gimp_mypaint_core_interpolate (GimpMypaintCore    *core,
                             GimpDrawable     *drawable,
                             GimpMypaintOptions *mypaint_options,
                             const GimpCoords *coords,
                             guint32           time)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (GIMP_IS_MYPAINT_OPTIONS (mypaint_options));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;

  GIMP_MYPAINT_CORE_GET_CLASS (core)->interpolate (core, drawable,
                                                 mypaint_options, time);
}

void
gimp_mypaint_core_set_current_coords (GimpMypaintCore    *core,
                                    const GimpCoords *coords)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->cur_coords = *coords;
}

void
gimp_mypaint_core_get_current_coords (GimpMypaintCore    *core,
                                    GimpCoords       *coords)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->cur_coords;

}

void
gimp_mypaint_core_set_last_coords (GimpMypaintCore    *core,
                                 const GimpCoords *coords)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  core->last_coords = *coords;
}

void
gimp_mypaint_core_get_last_coords (GimpMypaintCore *core,
                                 GimpCoords    *coords)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (coords != NULL);

  *coords = core->last_coords;
}

/**
 * gimp_mypaint_core_round_line:
 * @core:                 the #GimpMypaintCore
 * @options:              the #GimpMypaintOptions to use
 * @constrain_15_degrees: the modifier state
 *
 * Adjusts core->last_coords and core_cur_coords in preparation to
 * drawing a straight line. If @center_pixels is TRUE the endpoints
 * get pushed to the center of the pixels. This avoids artefacts
 * for e.g. the hard mode. The rounding of the slope to 15 degree
 * steps if ctrl is pressed happens, as does rounding the start and
 * end coordinates (which may be fractional in high zoom modes) to
 * the center of pixels.
 **/
void
gimp_mypaint_core_round_line (GimpMypaintCore    *core,
                            GimpMypaintOptions *mypaint_options,
                            gboolean          constrain_15_degrees)
{
  g_return_if_fail (GIMP_IS_MYPAINT_CORE (core));
  g_return_if_fail (GIMP_IS_MYPAINT_OPTIONS (mypaint_options));

  if (gimp_mypaint_options_get_brush_mode (mypaint_options) == GIMP_BRUSH_HARD)
    {
      core->last_coords.x = floor (core->last_coords.x) + 0.5;
      core->last_coords.y = floor (core->last_coords.y) + 0.5;
      core->cur_coords.x  = floor (core->cur_coords.x ) + 0.5;
      core->cur_coords.y  = floor (core->cur_coords.y ) + 0.5;
    }

  if (constrain_15_degrees)
    gimp_constrain_line (core->last_coords.x, core->last_coords.y,
                         &core->cur_coords.x, &core->cur_coords.y,
                         GIMP_CONSTRAIN_LINE_15_DEGREES);
}
#endif
