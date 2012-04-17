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

#include "paint-types.h"

#include "libgimpmath/gimpmath.h"
#include "gimpmypaintcore.hpp"
#include "gimpmypaintcoreundo.h"
};
#include "mypaintbrush-surface.hpp"
#include "mypaintbrush-stroke.hpp"

enum
{
  PROP_0,
  PROP_STROKE
};


static void   gimp_mypaint_core_undo_constructed  (GObject             *object);
static void   gimp_mypaint_core_undo_set_property (GObject             *object,
                                                 guint                property_id,
                                                 const GValue        *value,
                                                 GParamSpec          *pspec);
static void   gimp_mypaint_core_undo_get_property (GObject             *object,
                                                 guint                property_id,
                                                 GValue              *value,
                                                 GParamSpec          *pspec);

static void   gimp_mypaint_core_undo_pop          (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode,
                                                 GimpUndoAccumulator *accum);
static void   gimp_mypaint_core_undo_free         (GimpUndo            *undo,
                                                 GimpUndoMode         undo_mode);


G_DEFINE_TYPE (GimpMypaintCoreUndo, gimp_mypaint_core_undo, GIMP_TYPE_UNDO)

#define parent_class gimp_mypaint_core_undo_parent_class


static void
gimp_mypaint_core_undo_class_init (GimpMypaintCoreUndoClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  GimpUndoClass *undo_class   = GIMP_UNDO_CLASS (klass);

  object_class->constructed  = gimp_mypaint_core_undo_constructed;
  object_class->set_property = gimp_mypaint_core_undo_set_property;
  object_class->get_property = gimp_mypaint_core_undo_get_property;

  undo_class->pop            = gimp_mypaint_core_undo_pop;
  undo_class->free           = gimp_mypaint_core_undo_free;

  g_object_class_install_property (object_class, PROP_STROKE,
                                   g_param_spec_object ("stroke", NULL, NULL,
                                                        G_TYPE_POINTER,
                                                        (GParamFlags)(GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY)));
}

static void
gimp_mypaint_core_undo_init (GimpMypaintCoreUndo *undo)
{
}

static void
gimp_mypaint_core_undo_constructed (GObject *object)
{
  GimpMypaintCoreUndo *mypaint_core_undo = GIMP_MYPAINT_CORE_UNDO (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

}

static void
gimp_mypaint_core_undo_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  GimpMypaintCoreUndo *mypaint_core_undo = GIMP_MYPAINT_CORE_UNDO (object);

  switch (property_id)
    {
    case PROP_STROKE:
      mypaint_core_undo->stroke = g_value_get_pointer (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mypaint_core_undo_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  GimpMypaintCoreUndo *mypaint_core_undo = GIMP_MYPAINT_CORE_UNDO (object);

  switch (property_id)
    {
    case PROP_STROKE:
      g_value_set_pointer (value, mypaint_core_undo->stroke);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mypaint_core_undo_pop (GimpUndo              *undo,
                          GimpUndoMode           undo_mode,
                          GimpUndoAccumulator   *accum)
{
  GimpMypaintCoreUndo *mypaint_core_undo = GIMP_MYPAINT_CORE_UNDO (undo);

  GIMP_UNDO_CLASS (parent_class)->pop (undo, undo_mode, accum);

  /*  only pop if the core still exists  */
  if (mypaint_core_undo->stroke)
    {
    }
}

static void
gimp_mypaint_core_undo_free (GimpUndo     *undo,
                           GimpUndoMode  undo_mode)
{
  GimpMypaintCoreUndo *mypaint_core_undo = GIMP_MYPAINT_CORE_UNDO (undo);

  if (mypaint_core_undo->stroke) {
    Stroke* stroke = reinterpret_cast<Stroke*>(mypaint_core_undo->stroke);
    delete stroke;
    mypaint_core_undo->stroke = NULL;
  }

  GIMP_UNDO_CLASS (parent_class)->free (undo, undo_mode);
}
