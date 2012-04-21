

extern "C" {

#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpgradient.h"
#include "core/gimpmypaintinfo.h"

#include "gimpmypaintoptions.h"

#include "gimp-intl.h"

#include <cairo.h>
}

#include "mypaintbrush-surface.hpp"
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
Stroke::start(Brush* brush)
{
  g_assert (!finished);
  
  //TODO: record brush state and setting here.
  
  brush->new_stroke();
  coords.clear();
  this->brush = brush;
  
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

void 
Stroke::stop()
{
 // TODO:
  finished = true;
  total_painting_time += brush->stroke_total_painting_time;
}

bool 
Stroke::is_empty()
{
  return total_painting_time == 0;
}

void 
Stroke::render(Surface* surface)
{
  // TODO:
  for (std::vector<StrokeRecord>::iterator i = coords.begin(); i != coords.end(); i ++) {
    brush->stroke_to(surface, i->coords.x, i->coords.y, 
                     i->coords.pressure, 
                     i->coords.xtilt, i->coords.ytilt, i->dtime);
  }

}

void 
Stroke::copy_using_different_brush(Surface* surface, Brush* b)
{
  // TODO:
  for (std::vector<StrokeRecord>::iterator i = coords.begin(); i != coords.end(); i ++) {
    b->stroke_to(surface, i->coords.x, i->coords.y, 
                     i->coords.pressure, 
                     i->coords.xtilt, i->coords.ytilt, i->dtime);
  }

}
