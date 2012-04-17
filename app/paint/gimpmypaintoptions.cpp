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

extern "C" {
#include "config.h"

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpconfig/gimpconfig.h"

#include "paint-types.h"

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimpdynamics.h"
#include "core/gimpdynamicsoutput.h"
#include "core/gimpgradient.h"
#include "core/gimptoolinfo.h"

#include "gimpmypaintoptions.h"

#include "gimp-intl.h"

#define DEFAULT_BRUSH_SIZE             20.0
#define DEFAULT_BRUSH_ASPECT_RATIO     0.0
#define DEFAULT_BRUSH_ANGLE            0.0

#define DEFAULT_BRUSH_MODE             GIMP_MYPAINT_NORMAL

enum
{
  PROP_0,

  PROP_MYPAINT_INFO,

  PROP_BRUSH_SIZE,
  PROP_BRUSH_ASPECT_RATIO,
  PROP_BRUSH_ANGLE,

  PROP_BRUSH_MODE,

  PROP_BRUSH_VIEW_TYPE,
  PROP_BRUSH_VIEW_SIZE,
};


static void    gimp_mypaint_options_dispose          (GObject      *object);
static void    gimp_mypaint_options_finalize         (GObject      *object);
static void    gimp_mypaint_options_set_property     (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void    gimp_mypaint_options_get_property     (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);



G_DEFINE_TYPE (GimpMypaintOptions, gimp_mypaint_options, GIMP_TYPE_TOOL_OPTIONS)

#define parent_class gimp_mypaint_options_parent_class


static void
gimp_mypaint_options_class_init (GimpMypaintOptionsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose      = gimp_mypaint_options_dispose;
  object_class->finalize     = gimp_mypaint_options_finalize;
  object_class->set_property = gimp_mypaint_options_set_property;
  object_class->get_property = gimp_mypaint_options_get_property;

#if 0
  g_object_class_install_property (object_class, PROP_MYPAINT_INFO,
                                   g_param_spec_object ("mypaint-info",
                                                        NULL, NULL,
                                                        GIMP_TYPE_MYPAINT_INFO,
                                                        GParamFlags(GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY)));
#endif

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_SIZE,
                                   "brush-size", _("Brush Size"),
                                   1.0, 10000.0, DEFAULT_BRUSH_SIZE,
                                   (GParamFlags)(GIMP_PARAM_STATIC_STRINGS));

  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_ASPECT_RATIO,
                                   "brush-aspect-ratio", _("Brush Aspect Ratio"),
                                   -20.0, 20.0, DEFAULT_BRUSH_ASPECT_RATIO,
                                   GParamFlags(GIMP_PARAM_STATIC_STRINGS));
  GIMP_CONFIG_INSTALL_PROP_DOUBLE (object_class, PROP_BRUSH_ANGLE,
                                   "brush-angle", _("Brush Angle"),
                                   -180.0, 180.0, DEFAULT_BRUSH_ANGLE,
                                   GParamFlags(GIMP_PARAM_STATIC_STRINGS));

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BRUSH_MODE,
                                 "brush-mode", _("Every stamp has its own opacity"),
                                 GIMP_TYPE_MYPAINT_BRUSH_MODE,
                                 DEFAULT_BRUSH_MODE,
                                 GParamFlags(GIMP_PARAM_STATIC_STRINGS));

  GIMP_CONFIG_INSTALL_PROP_ENUM (object_class, PROP_BRUSH_VIEW_TYPE,
                                 "brush-view-type", NULL,
                                 GIMP_TYPE_VIEW_TYPE,
                                 GIMP_VIEW_TYPE_GRID,
                                 GParamFlags(GIMP_PARAM_STATIC_STRINGS));
  GIMP_CONFIG_INSTALL_PROP_INT (object_class, PROP_BRUSH_VIEW_SIZE,
                                "brush-view-size", NULL,
                                GIMP_VIEW_SIZE_TINY,
                                GIMP_VIEWABLE_MAX_BUTTON_SIZE,
                                GIMP_VIEW_SIZE_SMALL,
                                GParamFlags(GIMP_PARAM_STATIC_STRINGS));

}

static void
gimp_mypaint_options_init (GimpMypaintOptions *options)
{
  options->brush_mode_save = DEFAULT_BRUSH_MODE;
  options->brush_mode      = DEFAULT_BRUSH_MODE;
}

static void
gimp_mypaint_options_dispose (GObject *object)
{
  GimpMypaintOptions *options = GIMP_MYPAINT_OPTIONS (object);

/*
  if (options->mypaint_info)
    {
      g_object_unref (options->mypaint_info);
      options->mypaint_info = NULL;
    }
*/
  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_mypaint_options_finalize (GObject *object)
{
  GimpMypaintOptions *options = GIMP_MYPAINT_OPTIONS (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mypaint_options_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GimpMypaintOptions* options = GIMP_MYPAINT_OPTIONS (object);
  switch (property_id)
    {
//    case PROP_MYPAINT_INFO:
//      options->mypaint_info = GIMP_MYPAINT_INFO (g_value_dup_object (value));
//      break;

    case PROP_BRUSH_SIZE:
      options->brush_size = g_value_get_double (value);
      break;

    case PROP_BRUSH_ASPECT_RATIO:
      options->brush_aspect_ratio = g_value_get_double (value);
      break;

    case PROP_BRUSH_ANGLE:
      options->brush_angle = - 1.0 * g_value_get_double (value) / 360.0; /* let's make the angle mathematically correct */
      break;

    case PROP_BRUSH_MODE:
      options->brush_mode = (GimpMypaintBrushMode)(g_value_get_enum (value));
      break;

    case PROP_BRUSH_VIEW_TYPE:
      options->brush_view_type = (GimpViewType)(g_value_get_enum (value));
      break;

    case PROP_BRUSH_VIEW_SIZE:
      options->brush_view_size = (GimpViewSize)(g_value_get_int (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mypaint_options_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GimpMypaintOptions     *options           = GIMP_MYPAINT_OPTIONS (object);

  switch (property_id)
    {
//    case PROP_MYPAINT_INFO:
//      g_value_set_object (value, options->mypaint_info);
//      break;

    case PROP_BRUSH_SIZE:
      g_value_set_double (value, options->brush_size);
      break;

    case PROP_BRUSH_ASPECT_RATIO:
      g_value_set_double (value, options->brush_aspect_ratio);
      break;

    case PROP_BRUSH_ANGLE:
      g_value_set_double (value, - 1.0 * options->brush_angle * 360.0); /* mathematically correct -> intuitively correct */
      break;

    case PROP_BRUSH_MODE:
      g_value_set_enum (value, options->brush_mode);
      break;

    case PROP_BRUSH_VIEW_TYPE:
      g_value_set_enum (value, options->brush_view_type);
      break;

    case PROP_BRUSH_VIEW_SIZE:
      g_value_set_int (value, options->brush_view_size);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


GimpMypaintOptions *
gimp_mypaint_options_new (GimpToolInfo *tool_info/*GimpMypaintInfo *mypaint_info*/)
{
  GimpMypaintOptions *options;

//  g_return_val_if_fail (GIMP_IS_MYPAINT_INFO (mypaint_info), NULL);

  options = GIMP_MYPAINT_OPTIONS (g_object_new (GIMP_TYPE_MYPAINT_OPTIONS,
                          "gimp",       tool_info->gimp,
                          "name",       "Mypaint",
//                          "name",       gimp_object_get_name (mypaint_info),
//                          "mypaint-info", mypaint_info,
                          NULL));

  return options;
}

GimpMypaintBrushMode
gimp_mypaint_options_get_brush_mode (GimpMypaintOptions *mypaint_options)
{
  GimpDynamics       *dynamics;
  GimpDynamicsOutput *force_output;

  g_return_val_if_fail (GIMP_IS_MYPAINT_OPTIONS (mypaint_options), GIMP_MYPAINT_NORMAL);

  return GIMP_MYPAINT_NORMAL;
}

void
gimp_mypaint_options_copy_brush_props (GimpMypaintOptions *src,
                                     GimpMypaintOptions *dest)
{
  gdouble  brush_size;
  gdouble  brush_angle;
  gdouble  brush_aspect_ratio;

  g_return_if_fail (GIMP_IS_MYPAINT_OPTIONS (src));
  g_return_if_fail (GIMP_IS_MYPAINT_OPTIONS (dest));

  g_object_get (src,
                "brush-size", &brush_size,
                "brush-angle", &brush_angle,
                "brush-aspect-ratio", &brush_aspect_ratio,
                NULL);

  g_object_set (dest,
                "brush-size", brush_size,
                "brush-angle", brush_angle,
                "brush-aspect-ratio", brush_aspect_ratio,
                NULL);
}

};
