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
#include "core/gimplayer.h"
#include "core/gimpdrawableundo.h"

#include "gimpmypaintcoreundo.h"
#include "gimpmypaintoptions.h"

#include "gimpairbrush.h"

#include "gimp-intl.h"
};

#include "base/delegators.hpp"
#include "gimpmypaintcore.hpp"
#include "gimpmypaintcore-surface.hpp"
#include "gimpmypaintcore-postprocess.hpp"
#include "gimpmypaintcore-postprocess-filters.hpp"
#include "core/gimpmypaintbrush-private.hpp"

/*  local function prototypes  */
namespace GimpMypaint {

Core::Core()
{
  ID               = 0; //global_core_ID++;

  surface                = NULL;
  brush                  = NULL;
  stroke                 = NULL;
  post_processor         = NULL;
  option_changed_handler = NULL;
}


Core::~Core ()
{
  g_print("Core(%lx)::destructor\n", (gulong)this);
  cleanup ();
}

void Core::cleanup()
{
  g_print("Core(%lx)::cleanup\n", (gulong)this);
  if (option_changed_handler)
    delete option_changed_handler;
  option_changed_handler = NULL;
  
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

void 
Core::stroke_to (GimpDrawable* drawable, 
                 gdouble dtime, 
                 const GimpCoords* coords,
                 GimpMypaintOptions* options)
{
  bool split = false;
  
  if (options != this->options) {
    if (this->options && option_changed_handler) {
      delete option_changed_handler;
      option_changed_handler = NULL;
    }
    this->options = options;
    option_changed_handler = g_signal_connect_delegator(G_OBJECT(options), "notify", Delegator::delegator(this, &Core::option_changed));
  }
  
  // Prepare Brush.
  update_resource(options);
  
  /// from Document#stroke_to

  // Prepare Stroke object
  if (!stroke) {
    stroke = new Stroke();

    // PostProcessor Test.
    if (!post_processor || post_processor->is_done()) {
      if (post_processor)
        delete post_processor;
      post_processor = new PostProcessManager(this, drawable);
      PostProcessManager::Task* task;
      task = new SimulatePressureTask();
      post_processor->add_task(task);
      task = new RenderTask();
      post_processor->add_task(task);
    }

    if (!surface) {
      g_print("MypaintCore(%lx)::create new surface...\n", (gulong)this);
      surface = GimpMypaintSurface_new(drawable);
    } else if (!surface->is_surface_for(drawable)) {
      surface->end_session();
      g_print("MypaintCore(%lx)::delete surface...\n", (gulong)this);
      delete surface;
      g_print("MypaintCore(%lx)::recreate new surface...\n", (gulong)this);
      surface = GimpMypaintSurface_new(drawable);
    }

    GimpContext* context = GIMP_CONTEXT (options);

    // update color values
    // FIXME: Color should be updated when Foreground color is selected
    GimpRGB fg_color, bg_color;
    gboolean      use_gimp_brushmark;
    GimpBrush*    brushmark = NULL;
    gboolean      use_gimp_texture;
    GimpPattern*  texture = NULL;
    gboolean      floating_stroke = FALSE;
    gdouble       stroke_opacity = 1.0;
    gboolean      lock_alpha = FALSE;
    
    gimp_context_get_foreground(GIMP_CONTEXT(options), &fg_color);
    gimp_context_get_background(GIMP_CONTEXT(options), &bg_color);

    g_object_get(G_OBJECT(options), "use_gimp_brushmark", &use_gimp_brushmark, NULL);
    if (use_gimp_brushmark)
      brushmark = gimp_context_get_brush (GIMP_CONTEXT(options));

    g_object_get(G_OBJECT(options), "use_gimp_texture", &use_gimp_texture, NULL);
    if (use_gimp_texture)
      texture = gimp_context_get_pattern(GIMP_CONTEXT(options));

    g_object_get(G_OBJECT(options), "non_incremental", &floating_stroke, NULL);
    g_object_get(G_OBJECT(options), "stroke_opacity", &stroke_opacity, NULL);
    surface->set_stroke_opacity(stroke_opacity);

    if (GIMP_IS_LAYER (drawable))
      lock_alpha = gimp_layer_get_lock_alpha (GIMP_LAYER (drawable));

    // Setup stroke and reflect parameters to surface.
    stroke->setup(brush, fg_color, bg_color, 
                  brushmark, texture,
                  floating_stroke, stroke_opacity, 
                  lock_alpha);
    stroke->start(surface);
  }
  
  surface->set_coords(coords);
  stroke->record(dtime, coords);

  /// from Layer#stroke_to
  split = brush->stroke_to(surface, coords->x, coords->y, 
                           coords->pressure, 
                           coords->xtilt, coords->ytilt, dtime);

  if (post_processor && surface->is_dirty())
    post_processor->cancel();
  
  if (split)
    split_stroke();
}

void Core::split_stroke()
{
//  g_print("splitting stroke...\n");
  if (!stroke)
    return;

  bool dirty = surface->is_dirty();
  GimpDrawableUndo* undo = GIMP_DRAWABLE_UNDO(stroke->stop(surface));
  // push stroke to undo stack.

  if (post_processor && undo) {
    post_processor->add_stroke(stroke, undo);
    post_processor->start();
  } else {
    delete stroke;  
  }
  stroke = NULL;
  /*
  if (brush) {
    delete brush;
    brush = NULL;
  }
  */
  return;
}

void Core::update_resource(GimpMypaintOptions* options)
{
  GimpContext* context = GIMP_CONTEXT (options);
//  GimpMypaintBrush* myb = context->mypaint_brush;
  GimpMypaintBrush* myb = gimp_mypaint_options_get_current_brush (options);
  BrushPrivate *myb_priv = NULL;
#if 0  
  if (mypaint_brush != myb) {
    g_print("Other brush is selected.\n");
    delete brush;
    brush = NULL;
  }
#endif
  if (!brush) {
    g_print("MypaintCore(%lx)::Create brush object\n", (gulong)this);
    brush = new Brush();
    option_changed(G_OBJECT(options), NULL);
    mypaint_brush = myb;
    brush->reset();
  }

}

void Core::reset_brush()
{
  if (brush) {
    brush->reset();
  }
}

void Core::option_changed(GObject* target, GParamSpec *pspec)
{
//  g_print("MypaintCore(%lx)::option_changed\n", (gulong)this);
  split_stroke();

  GimpMypaintOptions* options = GIMP_MYPAINT_OPTIONS(target);

  GimpMypaintBrush* myb = gimp_mypaint_options_get_current_brush (options);
  BrushPrivate *myb_priv = NULL;
  if (myb)
    myb_priv = reinterpret_cast<BrushPrivate*>(myb->p);


  // copy brush setting here.
  if (myb_priv) {
    for (int i = 0; i < BRUSH_MAPPING_COUNT; i ++) {
      Mapping* m = myb_priv->get_setting(i)->mapping;
      if (m)
        brush->copy_mapping(i, m);
      else {
        brush->set_mapping_n(i, 0, 0);
        brush->set_base_value(i, myb_priv->get_setting(i)->base_value);
      }
    }
  }
}

}; // namespace GimpMypaint


#if 0
/**
 * gimp_mypaint_core_round_line:
 * @core:                 the #Core
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
gimp_mypaint_core_round_line (Core    *core,
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
