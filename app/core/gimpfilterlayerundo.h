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

#ifndef __GIMP_FILTER_LAYER_UNDO_H__
#define __GIMP_FILTER_LAYER_UNDO_H__


#include "gimpitemundo.h"

#ifdef __cplusplus
extern "C" {
#endif

struct GimpFilterLayerUndo
{
  GimpItemUndo       parent_instance;
};

struct GimpFilterLayerUndoClass
{
  GimpItemUndoClass  parent_class;
};


GType   gimp_filter_layer_undo_get_type (void) G_GNUC_CONST;

#ifdef __cplusplus
}
#include "base/glib-cxx-impl.hpp"
typedef GLib::Traits<GimpFilterLayerUndo, GimpFilterLayerUndoClass, gimp_filter_layer_undo_get_type> FilterLayerUndoTraits;
#endif

#endif /* __GIMP_FILTER_LAYER_UNDO_H__ */
