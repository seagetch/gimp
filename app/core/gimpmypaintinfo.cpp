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
#if 0
extern "C" {
#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "tools/gimpmypainttool.h"
#include "paint/gimpmypaintoptions.h"

#include "gimp.h"
#include "gimpmypaintinfo.h"

static void    gimp_mypaint_info_dispose         (GObject       *object);
static void    gimp_mypaint_info_finalize        (GObject       *object);
static gchar * gimp_mypaint_info_get_description (GimpViewable  *viewable,
                                                gchar        **tooltip);


G_DEFINE_TYPE (GimpMypaintInfo, gimp_mypaint_info, GIMP_TYPE_VIEWABLE)

#define parent_class gimp_mypaint_info_parent_class


static void
gimp_mypaint_info_class_init (GimpMypaintInfoClass *klass)
{
  GObjectClass      *object_class   = G_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class = GIMP_VIEWABLE_CLASS (klass);

  object_class->dispose           = gimp_mypaint_info_dispose;
  object_class->finalize          = gimp_mypaint_info_finalize;

  viewable_class->get_description = gimp_mypaint_info_get_description;
}

static void
gimp_mypaint_info_init (GimpMypaintInfo *mypaint_info)
{
  mypaint_info->gimp          = NULL;
  mypaint_info->mypaint_type    = G_TYPE_NONE;
  mypaint_info->blurb         = NULL;
  mypaint_info->mypaint_options = NULL;
}

static void
gimp_mypaint_info_dispose (GObject *object)
{
  GimpMypaintInfo *mypaint_info = GIMP_MYPAINT_INFO (object);

  if (mypaint_info->mypaint_options)
    {
      g_object_run_dispose (G_OBJECT (mypaint_info->mypaint_options));
      g_object_unref (mypaint_info->mypaint_options);
      mypaint_info->mypaint_options = NULL;
    }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static void
gimp_mypaint_info_finalize (GObject *object)
{
  GimpMypaintInfo *mypaint_info = GIMP_MYPAINT_INFO (object);

  if (mypaint_info->blurb)
    {
      g_free (mypaint_info->blurb);
      mypaint_info->blurb = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gchar *
gimp_mypaint_info_get_description (GimpViewable  *viewable,
                                 gchar        **tooltip)
{
  GimpMypaintInfo *mypaint_info = GIMP_MYPAINT_INFO (viewable);

  return g_strdup (mypaint_info->blurb);
}

GimpMypaintInfo *
gimp_mypaint_info_new (Gimp        *gimp,
                     GType        mypaint_type,
                     GType        mypaint_options_type,
                     const gchar *identifier,
                     const gchar *blurb,
                     const gchar *stock_id)
{
  GimpMypaintInfo *mypaint_info;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);
  g_return_val_if_fail (identifier != NULL, NULL);
  g_return_val_if_fail (blurb != NULL, NULL);
  g_return_val_if_fail (stock_id != NULL, NULL);

  mypaint_info = GIMP_MYPAINT_INFO (g_object_new (GIMP_TYPE_MYPAINT_INFO,
                                      "name",     identifier,
                                      "stock-id", stock_id,
                                      NULL));

  mypaint_info->gimp               = gimp;
  mypaint_info->mypaint_type         = mypaint_type;
  mypaint_info->mypaint_options_type = mypaint_options_type;
  mypaint_info->blurb              = g_strdup (blurb);

  mypaint_info->mypaint_options      = gimp_mypaint_options_new (mypaint_info);

  return mypaint_info;
}

void
gimp_mypaint_info_set_standard (Gimp          *gimp,
                              GimpMypaintInfo *mypaint_info)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (! mypaint_info || GIMP_IS_MYPAINT_INFO (mypaint_info));

  if (mypaint_info != gimp->standard_mypaint_info)
    {
      if (gimp->standard_mypaint_info)
        g_object_unref (gimp->standard_mypaint_info);

      gimp->standard_mypaint_info = mypaint_info;

      if (gimp->standard_mypaint_info)
        g_object_ref (gimp->standard_mypaint_info);
    }
}

GimpMypaintInfo *
gimp_mypaint_info_get_standard (Gimp *gimp)
{
  g_return_val_if_fail (GIMP_IS_GIMP (gimp), NULL);

  return gimp->standard_mypaint_info;
}
/*
GimpMypaintInfo* 
gimp_mypaint_info_get_by_name (Gimp *gimp, const gchar* name)
{
  return gimp_mypaint_info_new(gimp, GIMP_TYPE_MYPAINT_TOOL, GIMP_TYPE_MYPAINT_OPTION, "gimp-mypaint", "Mypaint", "gimp-tool-mypaint");
}
*/
};
#endif

