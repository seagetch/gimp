

extern "C" {

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimppattern.h"
#include "core/gimpimage.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpgradient.h"

#include "gimpmypaintoptions.h"

#include "gimp-intl.h"

#include <cairo.h>
}

#include "mypaintbrush-surface.hpp"
#include "gimpmypaintcore-surface.hpp"
#include "mypaintbrush-brush.hpp"
#include "mypaintbrush-stroke.hpp"
#include "gimpmypaintcore.hpp"

gint Stroke::_serial_number = 0;

Stroke::Stroke() : brush(NULL), finished(FALSE)
{
  serial_number = ++_serial_number;
}


Stroke::~Stroke() 
{
}

void 
Stroke::setup(Brush* brush,
              GimpRGB fg_color, GimpRGB bg_color, 
              GimpBrush* brushmark, GimpPattern* texture,
              bool floating_stroke, gdouble stroke_opacity, bool lock_alpha)
{
  g_assert (!finished);
  coords.clear();
  
  //TODO: record brush state and setting here.
  
  this->brush           = brush;

  // update color values
  // FIXME: Color should be updated when Foreground color is selected
  gimp_rgb_to_hsv (&fg_color, &fg_color_hsv);

  this->bg_color        = bg_color;

  // Attach gimp brush to the surface object.
  if (brushmark)
    g_object_ref(brushmark);
  this->brushmark       = brushmark;

  if (texture)
    g_object_ref(texture);
  this->texture         = texture;

  this->floating_stroke = floating_stroke;

  this->stroke_opacity  = stroke_opacity;
  this->lock_alpha      = lock_alpha;
}

void
Stroke::start(GimpMypaintSurface* surface)
{
  brush->new_stroke();

  brush->set_base_value(BRUSH_COLOR_H, fg_color_hsv.h);
  brush->set_base_value(BRUSH_COLOR_S, fg_color_hsv.s);
  brush->set_base_value(BRUSH_COLOR_V, fg_color_hsv.v);

  surface->set_bg_color(&bg_color);

  surface->set_brushmark(brushmark);
  surface->set_texture(texture);

  surface->set_floating_stroke(floating_stroke);
  surface->set_stroke_opacity(stroke_opacity);

  if (lock_alpha)
    brush->set_base_value(BRUSH_LOCK_ALPHA, 1.0);
  else
    brush->set_base_value(BRUSH_LOCK_ALPHA, 0.0);

  surface->begin_session();
}

void 
Stroke::record(gdouble dtime, 
               const GimpCoords* coord)
{
  // TODO:
  g_assert (!finished);
  StrokeRecord rec;
  rec.dtime  = dtime;
  rec.coords = *coord;
  coords.push_back(rec);
}

void*
Stroke::stop(GimpMypaintSurface* surface)
{
 // TODO:
  finished = true;
  total_painting_time += brush->stroke_total_painting_time;
  return surface->end_session();
}

bool 
Stroke::is_empty()
{
  return total_painting_time == 0;
}

void*
Stroke::render(GimpMypaintSurface* surface)
{
  start(surface);
  
  for (std::vector<StrokeRecord>::iterator i = coords.begin(); i != coords.end(); i ++) {
    surface->set_coords(&i->coords);
    brush->stroke_to(surface, i->coords.x, i->coords.y, 
                     i->coords.pressure, 
                     i->coords.xtilt, i->coords.ytilt, i->dtime);
  }

  return stop(surface);
}

void 
Stroke::copy_using_different_brush(GimpMypaintSurface* surface, Brush* b)
{
  // TODO:
  for (std::vector<StrokeRecord>::iterator i = coords.begin(); i != coords.end(); i ++) {
    surface->set_coords(&i->coords);
    b->stroke_to(surface, i->coords.x, i->coords.y, 
                     i->coords.pressure, 
                     i->coords.xtilt, i->coords.ytilt, i->dtime);
  }

}
