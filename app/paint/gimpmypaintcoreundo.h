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

#ifndef __GIMP_MYPAINT_CORE_UNDO_H__
#define __GIMP_MYPAINT_CORE_UNDO_H__


#include "core/gimpundo.h"


#define GIMP_TYPE_MYPAINT_CORE_UNDO            (gimp_mypaint_core_undo_get_type ())
#define GIMP_MYPAINT_CORE_UNDO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYPAINT_CORE_UNDO, GimpMypaintCoreUndo))
#define GIMP_MYPAINT_CORE_UNDO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYPAINT_CORE_UNDO, GimpMypaintCoreUndoClass))
#define GIMP_IS_MYPAINT_CORE_UNDO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYPAINT_CORE_UNDO))
#define GIMP_IS_MYPAINT_CORE_UNDO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYPAINT_CORE_UNDO))
#define GIMP_MYPAINT_CORE_UNDO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYPAINT_CORE_UNDO, GimpMypaintCoreUndoClass))


typedef struct _GimpMypaintCoreUndoClass GimpMypaintCoreUndoClass;

struct _GimpMypaintCoreUndo
{
  GimpUndo         parent_instance;
  gpointer         stroke;
};

struct _GimpMypaintCoreUndoClass
{
  GimpUndoClass  parent_class;
};


GType   gimp_mypaint_core_undo_get_type (void) G_GNUC_CONST;


#endif /* __GIMP_MYPAINT_CORE_UNDO_H__ */
