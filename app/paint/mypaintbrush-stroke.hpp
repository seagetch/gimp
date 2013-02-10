#ifndef __MYPAINT_BRUSH_STROKE_HPP__
#define __MYPAINT_BRUSH_STROKE_HPP__

#include "mypaintbrush-brush.hpp"
#include "mypaintbrush-surface.hpp"
#include "base/glib-cxx-utils.hpp"
#include "core/gimppattern.h"
#include "core/gimpbrush.h"
#include <vector>

DEFINE_REF_PTR(GimpPattern, GIMP_TYPE_PATTERN);
DEFINE_REF_PTR(GimpBrush, GIMP_TYPE_BRUSH);

class Stroke {
public:
  struct StrokeRecord {
    gdouble dtime;
    GimpCoords coords;
  };
private:
  static gint _serial_number;

  gdouble                total_painting_time;
  bool                   finished;
  gint                   serial_number;
  std::vector<StrokeRecord> coords;

  Brush*                 brush;
  GimpHSV                fg_color_hsv;
  GimpRGB                bg_color;
  Ref::GimpPattern       texture;
  Ref::GimpBrush         brushmark;
  bool                   floating_stroke;
  gdouble                stroke_opacity;
  bool                   lock_alpha;


public:
  Stroke();
  ~Stroke();

  void setup(Brush* brush, GimpRGB fg_color, GimpRGB bg_color, 
             GimpBrush* brushmark, GimpPattern* texture,
             bool floating_stroke, gdouble stroke_opacity, bool lock_alpha);
  void start(GimpMypaintSurface* surface);
  void record(gdouble dtime, 
              const GimpCoords* coord);
  void* stop(GimpMypaintSurface* surface);
  bool is_empty();
  
  void* render(GimpMypaintSurface* surface);
  void copy_using_different_brush(GimpMypaintSurface* surface, Brush* b);

  std::vector<StrokeRecord>& get_records() { return coords; }
  const std::vector<StrokeRecord>& get_records() const { return coords; }
};

#endif /* __MYPAINT_BRUSH_STROKE_HPP__ */
