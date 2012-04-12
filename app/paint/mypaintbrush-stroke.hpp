#ifndef __MYPAINT_BRUSH_STROKE_HPP__
#define __MYPAINT_BRUSH_STROKE_HPP__

#include "mypaintbrush-brush.hpp"
#include "mypaintbrush-surface.hpp"
#include <vector>

class Stroke {
  struct StrokeRecord {
    gdouble dtime;
    GimpCoords coords;
  };

  static gint _serial_number;

  Brush*                 brush;
  gdouble                total_painting_time;
  bool                   finished;
  gint                   serial_number;
  std::vector<StrokeRecord> coords;

public:
  Stroke();
  ~Stroke();

  void start(Brush* brush);
  void record(gdouble dtime, 
              const GimpCoords* coord);
  void stop();
  bool is_empty();
  void render(Surface* surface);
  void copy_using_different_brush(Surface* surface, Brush* b);
};

#endif /* __MYPAINT_BRUSH_STROKE_HPP__ */
