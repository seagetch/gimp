/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmypaint_brushselect.c
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "widgets-types.h"

#include "base/temp-buf.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpmypaintbrush.h"
#include "core/gimpparamspecs.h"

#include "pdb/gimppdb.h"

#include "gimpmypaintbrushfactoryview.h"
#include "gimpmypaintbrushselect.h"
#include "gimpcontainerbox.h"
#include "gimpspinscale.h"
#include "gimpwidgets-constructors.h"

#include "gimp-intl.h"


enum
{
  PROP_0,
  PROP_OPACITY,
  PROP_PAINT_MODE,
  PROP_SPACING
};


static void          gimp_mypaint_brush_select_constructed  (GObject         *object);
static void          gimp_mypaint_brush_select_set_property (GObject         *object,
                                                     guint            property_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);

static GValueArray * gimp_mypaint_brush_select_run_callback (GimpPdbDialog   *dialog,
                                                     GimpObject      *object,
                                                     gboolean         closing,
                                                     GError         **error);


G_DEFINE_TYPE (GimpMypaintBrushSelect, gimp_mypaint_brush_select, GIMP_TYPE_PDB_DIALOG)

#define parent_class gimp_mypaint_brush_select_parent_class


static void
gimp_mypaint_brush_select_class_init (GimpMypaintBrushSelectClass *klass)
{
  GObjectClass       *object_class = G_OBJECT_CLASS (klass);
  GimpPdbDialogClass *pdb_class    = GIMP_PDB_DIALOG_CLASS (klass);

  object_class->constructed  = gimp_mypaint_brush_select_constructed;
  object_class->set_property = gimp_mypaint_brush_select_set_property;

  pdb_class->run_callback    = gimp_mypaint_brush_select_run_callback;
}

static void
gimp_mypaint_brush_select_init (GimpMypaintBrushSelect *select)
{
}

static void
gimp_mypaint_brush_select_constructed (GObject *object)
{
  GimpPdbDialog   *dialog = GIMP_PDB_DIALOG (object);
  GtkWidget       *content_area;

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  dialog->view =
    gimp_mypaint_brush_factory_view_new (GIMP_VIEW_TYPE_GRID,
                                 dialog->context->gimp->mypaint_brush_factory,
                                 dialog->context,
                                 GIMP_VIEW_SIZE_MEDIUM, 1,
                                 dialog->menu_factory);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (GIMP_CONTAINER_EDITOR (dialog->view)->view),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2),
                                       5 * (GIMP_VIEW_SIZE_MEDIUM + 2));

  gtk_container_set_border_width (GTK_CONTAINER (dialog->view), 12);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_box_pack_start (GTK_BOX (content_area), dialog->view, TRUE, TRUE, 0);
  gtk_widget_show (dialog->view);
}

static void
gimp_mypaint_brush_select_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static GValueArray *
gimp_mypaint_brush_select_run_callback (GimpPdbDialog  *dialog,
                                GimpObject     *object,
                                gboolean        closing,
                                GError        **error)
{
  GimpMypaintBrush   *mypaint_brush = GIMP_MYPAINT_BRUSH (object);
  GimpArray   *array;
  GValueArray *return_vals;
  g_print("gimp_mypaint_brush_select_run_callback\n");
#if 0
  array = gimp_array_new (temp_buf_get_data (mypaint_brush->mask),
                          temp_buf_get_data_size (mypaint_brush->mask),
                          TRUE);

  return_vals =
    gimp_pdb_execute_procedure_by_name (dialog->pdb,
                                        dialog->caller_context,
                                        NULL, error,
                                        dialog->callback_name,
                                        G_TYPE_STRING,        gimp_object_get_name (object),
                                        G_TYPE_DOUBLE,        100,//gimp_context_get_opacity (dialog->context) * 100.0,
                                        GIMP_TYPE_INT32,      1,//GIMP_MYPAINT_BRUSH_SELECT (dialog)->spacing,
                                        GIMP_TYPE_INT32,      gimp_context_get_paint_mode (dialog->context),
                                        GIMP_TYPE_INT32,      1,//mypaint_brush->mask->width,
                                        GIMP_TYPE_INT32,      1,//mypaint_brush->mask->height,
                                        GIMP_TYPE_INT32,      array->length,
                                        GIMP_TYPE_INT8_ARRAY, array,
                                        GIMP_TYPE_INT32,      closing,
                                        G_TYPE_NONE);

  gimp_array_free (array);
#else
  return_vals = NULL;
#endif

  return return_vals;
}
