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

#ifndef __GIMP_MYPAINT_INFO_H__
#define __GIMP_MYPAINT_INFO_H__


#include "gimpviewable.h"


#define GIMP_TYPE_MYPAINT_INFO            (gimp_mypaint_info_get_type ())
#define GIMP_MYPAINT_INFO(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_MYPAINT_INFO, GimpMypaintInfo))
#define GIMP_MYPAINT_INFO_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_MYPAINT_INFO, GimpMypaintInfoClass))
#define GIMP_IS_MYPAINT_INFO(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_MYPAINT_INFO))
#define GIMP_IS_MYPAINT_INFO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_MYPAINT_INFO))
#define GIMP_MYPAINT_INFO_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_MYPAINT_INFO, GimpMypaintInfoClass))


typedef struct _GimpMypaintInfoClass GimpMypaintInfoClass;

struct _GimpMypaintInfo
{
  GimpViewable      parent_instance;

  Gimp             *gimp;

  GType             mypaint_type;
  GType             mypaint_options_type;

  gchar            *blurb;

  GimpMypaintOptions *mypaint_options;
};

struct _GimpMypaintInfoClass
{
  GimpViewableClass  parent_class;
};


GType           gimp_mypaint_info_get_type     (void) G_GNUC_CONST;

GimpMypaintInfo * gimp_mypaint_info_new          (Gimp          *gimp,
                                              GType          mypaint_type,
                                              GType          mypaint_options_type,
                                              const gchar   *identifier,
                                              const gchar   *blurb,
                                              const gchar   *stock_id);

void            gimp_mypaint_info_set_standard (Gimp          *gimp,
                                              GimpMypaintInfo *mypaint_info);
GimpMypaintInfo * gimp_mypaint_info_get_standard (Gimp          *gimp);


#endif  /*  __GIMP_MYPAINT_INFO_H__  */
