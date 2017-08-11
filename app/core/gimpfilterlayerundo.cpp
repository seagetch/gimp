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

#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"
#include "base/glib-cxx-impl.hpp"

extern "C" {
#include "config.h"

#include <gegl.h>
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"
#include "base/glib-cxx-impl.hpp"

#include "core-types.h"

#include "gimpimage.h"
#include "gimpitemundo.h"
}

#include "gimpfilterlayer.h"
#include "gimpfilterlayerundo.h"
#include <type_traits>

struct FilterLayerUndo;

namespace GLib {

struct FilterLayerUndo : virtual public ImplBase
{
  typedef Traits<GimpFilterLayerUndo> traits;
  FilterLayerUndo(GObject* obj) : ImplBase(obj) {
  };

  GimpImageBaseType  prev_type;

  static void class_init(GimpFilterLayerUndoClass* klass);

  virtual void constructed();
  virtual void pop        (GimpUndoMode  undo_mode, GimpUndoAccumulator * accum);
};

};

extern const char gimp_filter_layer_undo_name[] = "GimpFilterLayerUndo";
using Class = GLib::GClass<gimp_filter_layer_undo_name,
                           GLib::UseCStructs<GimpItemUndo, GimpFilterLayerUndo >,
                           GLib::FilterLayerUndo>;

GType gimp_filter_layer_undo_get_type() {
  return Class::get_type();
}

#define bind_to_class(klass, method, impl)  Class::__(&klass->method).bind<&impl::method>()

void
GLib::FilterLayerUndo::class_init (GimpFilterLayerUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  bind_to_class (object_class, constructed, GLib::FilterLayerUndo);

  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);
  bind_to_class (undo_class, pop, GLib::FilterLayerUndo);

  ImplBase::class_init<GimpFilterLayerUndoClass, FilterLayerUndo>(klass);
}

void GLib::FilterLayerUndo::constructed ()
{
  GObject     *group;

  if (G_OBJECT_CLASS (Class::parent_class)->constructed)
    G_OBJECT_CLASS (Class::parent_class)->constructed (g_object);

  g_assert (FilterLayerInterface::is_instance (GIMP_ITEM_UNDO (g_object)->item));

  group = G_OBJECT(GIMP_ITEM_UNDO (g_object)->item);

  switch (GIMP_UNDO (g_object)->undo_type)
    {
    case GIMP_UNDO_GROUP_LAYER_SUSPEND:
    case GIMP_UNDO_GROUP_LAYER_RESUME:
      break;

    case GIMP_UNDO_GROUP_LAYER_CONVERT:
      prev_type = (GimpImageBaseType)GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (group)));
      break;

    default:
      g_assert_not_reached ();
    }
}

void GLib::FilterLayerUndo::pop (GimpUndoMode         undo_mode,
                                   GimpUndoAccumulator *accum)
{
  GimpUndo*            undo;
  auto i_filter = FilterLayerInterface::cast(GIMP_ITEM_UNDO(g_object)->item);
  undo  = GIMP_UNDO(g_object);

  GIMP_UNDO_CLASS (Class::parent_class)->pop (undo, undo_mode, accum);

  switch (GIMP_UNDO(g_object)->undo_type){
  case GIMP_UNDO_GROUP_LAYER_SUSPEND:
  case GIMP_UNDO_GROUP_LAYER_RESUME:
    if ((undo_mode       == GIMP_UNDO_MODE_UNDO &&
        undo->undo_type == GIMP_UNDO_GROUP_LAYER_SUSPEND) ||
        (undo_mode       == GIMP_UNDO_MODE_REDO &&
            undo->undo_type == GIMP_UNDO_GROUP_LAYER_RESUME)) {
      /*    resume group layer auto-resizing  */

      i_filter->resume_resize(FALSE);
    } else {
      /*  suspend group layer auto-resizing  */

      i_filter->suspend_resize (FALSE);
    }
    break;

  case GIMP_UNDO_GROUP_LAYER_CONVERT:
  {
    GimpImageBaseType type;

    type = (GimpImageBaseType)GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE (g_object)));
    gimp_drawable_convert_type (GIMP_DRAWABLE (g_object), NULL,
        prev_type, FALSE);
    prev_type = type;
  }
  break;

  default:
    g_assert_not_reached ();
  }
}
