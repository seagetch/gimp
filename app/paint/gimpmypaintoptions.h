/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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

#ifndef __GIMP_MYPAINT_OPTIONS_H__
#define __GIMP_MYPAINT_OPTIONS_H__


#include "core/gimptooloptions.h"


#define GIMP_MYPAINT_OPTIONS_CONTEXT_MASK GIMP_CONTEXT_FOREGROUND_MASK | \
                                        GIMP_CONTEXT_BACKGROUND_MASK | \
                                        GIMP_CONTEXT_OPACITY_MASK    | \
                                        GIMP_CONTEXT_MYPAINT_MODE_MASK | \
                                        GIMP_CONTEXT_BRUSH_MASK      | \
                                        GIMP_CONTEXT_DYNAMICS_MASK

#define GIMP_TYPE_MYPAINT_OPTIONS            (gimp_mypaint_options_get_type ())
#define GIMP_MYPAINT_OPTIONS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYPAINT_OPTIONS, GimpMypaintOptions))
#define GIMP_MYPAINT_OPTIONS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYPAINT_OPTIONS, GimpMypaintOptionsClass))
#define GIMP_IS_MYPAINT_OPTIONS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYPAINT_OPTIONS))
#define GIMP_IS_MYPAINT_OPTIONS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYPAINT_OPTIONS))
#define GIMP_MYPAINT_OPTIONS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYPAINT_OPTIONS, GimpMypaintOptionsClass))


typedef struct _GimpMypaintOptionsClass GimpMypaintOptionsClass;

struct _GimpMypaintOptions
{
  GimpToolOptions           parent_instance;

  GimpMypaintInfo            *mypaint_info;

  gdouble                   brush_size;
  gdouble                   brush_angle;
  gdouble                   brush_aspect_ratio;

  GimpMypaintBrushMode      brush_mode;
  GimpMypaintBrushMode      brush_mode_save;

  GimpViewType              brush_view_type;
  GimpViewSize              brush_view_size;
};

struct _GimpMypaintOptionsClass
{
  GimpToolOptionsClass  parent_instance;
};


GType              gimp_mypaint_options_get_type (void) G_GNUC_CONST;

GimpMypaintOptions * gimp_mypaint_options_new      (GimpMypaintInfo    *mypaint_info);

GimpMypaintBrushMode
gimp_mypaint_options_get_brush_mode (GimpMypaintOptions *mypaint_options);

void    gimp_mypaint_options_copy_brush_props    (GimpMypaintOptions *src,
                                                GimpMypaintOptions *dest);

#endif  /*  __GIMP_MYPAINT_OPTIONS_H__  */
