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

#ifndef __GIMP_MYPAINT_CORE_POSTPROCESS_HPP__
#define __GIMP_MYPAINT_CORE_POSTPROCESS_HPP__

#include "base/glib-cxx-utils.hpp"
#include "base/delegators.hpp"
#include "core/core-types.h"
#include <vector>

DEFINE_REF_PTR(GimpDrawable, GIMP_TYPE_DRAWABLE);
DEFINE_REF_PTR(GimpMypaintOptions, GIMP_TYPE_MYPAINT_OPTIONS);

namespace GimpMypaint {

class PostProcessManager
{
public:
  class Task {
  public:
    virtual void run(Core* core,
                     GimpDrawable* drawable,
                     Stroke* stroke) = 0;
  };
private:
  class StrokeHolder {
    Stroke* stroke;
    GimpDrawableUndo* undo;
    Delegator::Connection* connection;
  public:
    StrokeHolder(Stroke* s, GimpDrawableUndo* u); 
    StrokeHolder(const StrokeHolder& src);
    ~StrokeHolder();
    Stroke*           get_stroke() { return stroke; }
    GimpDrawableUndo* get_undo()   { return undo; }
    void on_pop(GObject* pop, GimpUndoType undo_type, gpointer accum);
  };
  Core*                      core;
  Ref::GimpDrawable          drawable;
  TileManager*               manager;
  
  gint                 idle_task_id;
  gint                 timeout_task_id;
  gboolean             done;
  std::vector<StrokeHolder> strokes;
  std::vector<Task*>   tasks;   // FIXME: tasks must be deleted when manager is deleted.
protected:
  static gboolean on_idle_callback(gpointer data);
  static gboolean on_timeout_callback(gpointer data);

  gboolean on_idle();
  gboolean on_timeout();

  void reset_drawable();
  void merge_undo_tiles(TileManager* undo_tiles);
public:
  PostProcessManager(Core* core, 
                     GimpDrawable* drawable);
  ~PostProcessManager();

  void add_task(Task* task);
  void add_stroke(Stroke* stroke, GimpDrawableUndo* undo);
  void start();
  void cancel();
  bool is_done();
};

}; // namespace GimpMypaint
#endif  /*  __GIMP_MYPAINT_CORE_POSTPROCESS_HPP__  */
