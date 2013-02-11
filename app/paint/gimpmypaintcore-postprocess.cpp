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
#include <glib-object.h>
#include <glib/gprintf.h>
  
#include <string.h>
#include <math.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/base-types.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "core/gimp.h"
#include "core/core-types.h"
#include "core/gimp-utils.h"
#include "core/gimpcontainer.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpundo.h"
#include "core/gimpundostack.h"
#include "core/gimpdrawableundo.h"
#include "core/gimpimage-undo.h"

#include "gimp-intl.h"
};

#include "base/delegators.hpp"
#include "gimpmypaintcore.hpp"
#include "gimpmypaintcore-postprocess.hpp"


namespace GimpMypaint {
////////////////////////////////////////////////////////////////////////////////
PostProcessManager::StrokeHolder::StrokeHolder(Stroke* s, GimpDrawableUndo* u) 
        : stroke(s), undo(u), connection(NULL) 
{
  if (undo) {
    g_object_ref(G_OBJECT(undo));
    connection = g_signal_connect_delegator(G_OBJECT(undo), "pop", Delegator::delegator(this, &StrokeHolder::on_pop));
  }
}

PostProcessManager::StrokeHolder::StrokeHolder(const StrokeHolder& src) {
  stroke     = src.stroke;
  undo       = src.undo;
  connection = NULL;
  StrokeHolder* force_write_to_src = const_cast<StrokeHolder*>(&src);
  force_write_to_src->stroke = NULL;
  force_write_to_src->undo   = NULL;

  if (force_write_to_src->connection) {
    delete force_write_to_src->connection;
    force_write_to_src->connection = NULL;
  }
  if (undo) {
    connection = g_signal_connect_delegator(G_OBJECT(undo), "pop", Delegator::delegator(this, &StrokeHolder::on_pop));
  }
}
PostProcessManager::StrokeHolder::~StrokeHolder() {
  if (stroke) {
    delete stroke;
    stroke = NULL;
  }
  if (connection) {
    delete connection;
    connection = NULL;
  }
  if (undo) {
    g_object_unref(G_OBJECT(undo));
    undo = NULL;
  }
}

void
PostProcessManager::StrokeHolder::on_pop(GObject* pop, GimpUndoType undo_type, gpointer accum)
{
  g_return_if_fail (G_OBJECT(undo) == pop);
  delete stroke;
  g_object_unref(pop);
  stroke = NULL;
  undo   = NULL;
}

////////////////////////////////////////////////////////////////////////////////
PostProcessManager::PostProcessManager(Core* core, 
                                       GimpDrawable* drawable)
  : idle_task_id(0), timeout_task_id(0), done(false)
{
  g_object_ref(drawable);
  
  this->core     = core;
  this->drawable = drawable;
  guint w, h, b;
  w = gimp_item_get_width(GIMP_ITEM(drawable));
  h = gimp_item_get_height(GIMP_ITEM(drawable));
  b = gimp_drawable_bytes(drawable);
  this->manager  = tile_manager_new(w, h, b);
}

PostProcessManager::~PostProcessManager()
{
  cancel();
  if (manager)
    tile_manager_unref(manager);
}

void 
PostProcessManager::add_task(PostProcessManager::Task* task)
{
  tasks.push_back(task);
}

void 
PostProcessManager::add_stroke(Stroke* stroke, GimpDrawableUndo* undo)
{
  cancel();
  TileManager* undo_tiles = undo->tiles;
  strokes.push_back(StrokeHolder(stroke, undo));
  merge_undo_tiles(undo_tiles);
}

void 
PostProcessManager::cancel()
{
  if (timeout_task_id) {
    g_source_remove(timeout_task_id);
    timeout_task_id = 0;
  }
  if (idle_task_id) {
    g_source_remove(idle_task_id);
    idle_task_id = 0;
  }
}

void 
PostProcessManager::start()
{
  cancel();
  done = false;
  timeout_task_id = g_timeout_add(500, (GSourceFunc)(PostProcessManager::on_timeout_callback), this);
  g_print("PostProcessManager::start()\n");
}

bool 
PostProcessManager::is_done()
{
  return done;
}

void 
PostProcessManager::reset_drawable()
{
//  g_return_if_fail (drawable.valid());
  g_return_if_fail (manager != NULL);

  TileManager* drawable_tiles = gimp_drawable_get_tiles(drawable);
  tile_manager_merge_over (drawable_tiles, manager);
  guint w, h;
  w = gimp_item_get_width(GIMP_ITEM(&*drawable));
  h = gimp_item_get_height(GIMP_ITEM(&*drawable));
  gimp_drawable_update (drawable, 0, 0, w, h);
}

void
PostProcessManager::merge_undo_tiles(TileManager* undo_tiles)
{
  tile_manager_merge_under (manager, undo_tiles);
}

gboolean 
PostProcessManager::on_idle()
{
  g_print("PostProcessManager::on_idle\n");
  // Revert drawable to previous state
  reset_drawable();
  for (std::vector<StrokeHolder>::iterator s = strokes.begin(); s < strokes.end(); s ++) {
    Stroke* stroke = s->get_stroke();
    if (!stroke)
      continue;
    for (std::vector<Task*>::iterator t = tasks.begin(); t < tasks.end(); t ++) {
      Task* task = *t;
      task->run(core, drawable, stroke);
    }
    GimpImage* image = gimp_item_get_image(GIMP_ITEM(&*drawable));
    GimpUndoStack* stack = gimp_image_get_undo_stack(image);
    gimp_container_remove(GIMP_CONTAINER(stack->undos), GIMP_OBJECT(s->get_undo()));
  }
  GimpImage* image = gimp_item_get_image(GIMP_ITEM(&*drawable));
  gimp_image_flush (image);
  done = TRUE;
  return FALSE;
}

gboolean 
PostProcessManager::on_timeout()
{
  g_print("PostProcessManager::on_timeout()\n");
  idle_task_id = g_idle_add((GSourceFunc)(PostProcessManager::on_idle_callback), this);
  return FALSE;
}

gboolean 
PostProcessManager::on_idle_callback(gpointer data)
{
  g_print("PostProcessManager::on_idle_callback(%lx)\n",(unsigned long)data);
  PostProcessManager* manager = reinterpret_cast<PostProcessManager*>(data);
  return manager->on_idle();
}

gboolean 
PostProcessManager::on_timeout_callback(gpointer data)
{
  g_print("PostProcessManager::on_timeout_callback(%lx)\n",(unsigned long)data);
  PostProcessManager* manager = reinterpret_cast<PostProcessManager*>(data);
  return manager->on_timeout();
}
////////////////////////////////////////////////////////////////////////////////

}; // namespace GimpMypaint