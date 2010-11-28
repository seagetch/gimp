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

#include "config.h"

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "tools-types.h"

#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptooloptions.h"
#include "core/gimpdatafactory.h"
#include "core/gimpbrush.h"

#include "widgets/gimpviewrenderer.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpcontainerbox.h"

#include "gimptooloptions-gui.h"
#include "gimpbrushoptions-gui.h"

#include "gimp-intl.h"


static void spacing_changed (GimpBrush *brush, gpointer   data);
static void spacing_update (GtkAdjustment *adjustment, gpointer       data);

/*  private functions  */
typedef struct _BrushDialogPrivate BrushDialogPrivate;
struct _BrushDialogPrivate
{
  GtkAdjustment *adj;
  GimpContainer *container;
  GimpContext   *context;
  gint           spacing_changed_handler_id;
};

static BrushDialogPrivate *
brush_dialog_private_new (void)
{
  BrushDialogPrivate *p = g_new0 (BrushDialogPrivate, 1);
  return p;
}

static void 
brush_dialog_private_set_adjustment (BrushDialogPrivate *p, GtkAdjustment *adj)
{
  p->adj = adj;
  g_object_ref (G_OBJECT (p->adj));
}

static void 
brush_dialog_private_set_container (BrushDialogPrivate *p, GimpContainer *container)
{
  p->container = container;
  g_object_ref (G_OBJECT (p->container));
}

static void 
brush_dialog_private_set_context (BrushDialogPrivate *p, GimpContext *context)
{
  p->context = context;
  g_object_ref (G_OBJECT (p->context));
}

static void
brush_dialog_private_destroy (gpointer data)
{
  BrushDialogPrivate *p = (BrushDialogPrivate*)data;

  if (p->spacing_changed_handler_id)
    {
      gimp_container_remove_handler (p->container,
                                     p->spacing_changed_handler_id);
      p->spacing_changed_handler_id = 0;
    }

  if (p->adj)
    {
      g_object_unref (G_OBJECT (p->adj));
      p->adj = NULL;
    }

  if (p->container)
    {
      g_object_unref (G_OBJECT (p->container));
      p->container = NULL;
    }

  if (p->context)
    {
      g_object_unref (G_OBJECT (p->context));
      p->context = NULL;
    }

  if (p)
    g_free (p);
}

static void
spacing_changed (GimpBrush *brush,
                 gpointer   data)
{
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;
  GimpContext         *context;

  context = p->context;
  
  g_return_if_fail (G_IS_OBJECT (p->adj));
  g_return_if_fail (G_IS_OBJECT (context));
  g_return_if_fail (GIMP_IS_BRUSH (brush));


  if (brush == gimp_context_get_brush (context))
    {
      g_signal_handlers_block_matched (G_OBJECT (p->adj),
                                       G_SIGNAL_MATCH_DETAIL,
                                       0, g_quark_from_static_string ("value-changed"), NULL, NULL, NULL);
      gtk_adjustment_set_value (p->adj,
                                gimp_brush_get_spacing (brush));

      g_signal_handlers_unblock_matched (G_OBJECT (p->adj),
                                         G_SIGNAL_MATCH_DETAIL,
                                         0, g_quark_from_static_string ("value-changed"), NULL, NULL, NULL);
    }
}

static void
spacing_update (GtkAdjustment *adjustment,
                gpointer       data)
{
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;
  GimpContext         *context;
  GimpBrush           *brush;

  context = p->context;

  g_return_if_fail (G_IS_OBJECT (p->adj));
  g_return_if_fail (G_IS_OBJECT (context));

  brush = gimp_context_get_brush (context);
  g_return_if_fail (GIMP_IS_BRUSH (brush));
  
  if (brush)
    {
      g_signal_handlers_block_matched (brush,
                                       G_SIGNAL_MATCH_DETAIL,
                                       0, g_quark_from_static_string ("spacing-changed"), NULL, NULL, NULL);
    
      gimp_brush_set_spacing (brush, gtk_adjustment_get_value (p->adj));

      g_signal_handlers_unblock_matched (brush,
                                         G_SIGNAL_MATCH_DETAIL,
                                         0, g_quark_from_static_string ("spacing-changed"), NULL, NULL, NULL);
    }

}

static void
destroy_brush_popup_dialog (GtkWidget *widget, gpointer data)
{
  BrushDialogPrivate            *p = (BrushDialogPrivate*)data;
  
  if (p->spacing_changed_handler_id)
    {
      gimp_container_remove_handler (p->container,
                                     p->spacing_changed_handler_id);
      p->spacing_changed_handler_id = 0;
    }
}

static void
create_brush_popup_dialog (GtkWidget  *button,
                           GtkWidget **result,
                           GObject    *config,
                           gpointer    data)
{
  GimpContainer                 *container;
  GimpContext                   *context;
  GimpContainerEditor           *editor;
  GimpBrush                     *brush;
  GtkWidget                     *table;
  GimpViewType                   view_type = GIMP_VIEW_TYPE_GRID;
  GimpViewSize                   view_size = GIMP_VIEW_SIZE_SMALL;
  gint                           view_border_width = 1;
  gint                           default_view_size = GIMP_VIEW_SIZE_SMALL;
  GimpToolOptionsTableIncrement  inc = gimp_tool_options_table_increment (FALSE);
  GList                         *children;
  BrushDialogPrivate            *p = (BrushDialogPrivate*)data;
  GtkAdjustment                 *adj = NULL;

  context   = GIMP_CONTEXT (config);
  container = gimp_data_factory_get_container (context->gimp->brush_factory);
  brush     = gimp_context_get_brush (context);
  
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  *result    = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (*result);
  
  editor = GIMP_CONTAINER_EDITOR (g_object_new (GIMP_TYPE_CONTAINER_EDITOR, NULL));
  gimp_container_editor_construct (editor,
                                   view_type,
                                   container,
                                   context,
                                   view_size,
                                   view_border_width,
                                   NULL, NULL, NULL);

  gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                       FALSE);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (editor->view),
                                       6  * (default_view_size +
                                             2 * view_border_width),
                                       10 * (default_view_size +
                                             2 * view_border_width));

  if (GIMP_IS_EDITOR (editor->view))
    gimp_editor_set_show_name (GIMP_EDITOR (editor->view), FALSE);

  gtk_box_pack_start (GTK_BOX (*result), GTK_WIDGET (editor), TRUE, TRUE, 0);      
  gtk_widget_show (GTK_WIDGET (editor));
  
  table = gtk_table_new (4, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (*result), table, TRUE, TRUE, 0);      
  gtk_widget_show (table);
  
  gimp_tool_options_scale_entry_new (config, "brush-scale",
                                     GTK_TABLE (table),
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Scale:"),
                                     0.01, 0.1, 2,
                                     FALSE, 0.0, 0.0, TRUE, FALSE);
  gimp_tool_options_table_increment_next (&inc);

  gimp_tool_options_scale_entry_new (config, "brush-aspect-ratio",
                                     GTK_TABLE (table),
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Aspect ratio:"),
                                     0.01, 0.1, 2,
                                     FALSE, 0.0, 0.0, FALSE, FALSE);
  gimp_tool_options_table_increment_next (&inc);

  gimp_tool_options_scale_entry_new (config, "brush-angle",
                                     GTK_TABLE (table),
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Angle:"),
                                     1.0, 5.0, 2,
                                     FALSE, 0.0, 0.0, FALSE, FALSE);
  gimp_tool_options_table_increment_next (&inc);  

  adj = GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                              gimp_tool_options_table_increment_get_col (&inc),
                                              gimp_tool_options_table_increment_get_row (&inc),
                                              _("Spacing:"), -1, -1,
                                              gimp_brush_get_spacing (brush), 1.0, 200.0, 1.0, 10.0, 1,
                                              FALSE, 1.0, 5000.0,
                                              _("Percentage of width of brush"),
                                              NULL));

  brush_dialog_private_set_adjustment (p, adj);
  brush_dialog_private_set_container  (p, container);
  brush_dialog_private_set_context    (p, context); 

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (spacing_update),
                    p);
  p->spacing_changed_handler_id =
    gimp_container_add_handler (container, "spacing-changed",
                                G_CALLBACK (spacing_changed),
                                p);

  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, FALSE);

  g_signal_connect (GTK_WIDGET (*result), "destroy", G_CALLBACK(destroy_brush_popup_dialog), p);
}


/*  public functions  */
GtkWidget*
gimp_brush_button_with_popup (GObject *config)
{
  GimpContainer *container;
  GimpContext   *context;
  GimpViewSize   view_size = GIMP_VIEW_SIZE_SMALL;
  const gchar        *prop_name;
  GtkWidget     *label_widget;
  BrushDialogPrivate *p = NULL;

  context   = GIMP_CONTEXT (config);
  container = gimp_data_factory_get_container (context->gimp->brush_factory);
  
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE, NULL);

  prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

  label_widget = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                     context, view_size);
  gtk_widget_show (label_widget);
  
  p = brush_dialog_private_new ();

  return gimp_tool_options_button_with_popup (config, label_widget,
                                               create_brush_popup_dialog,
                                               p, brush_dialog_private_destroy);
}
