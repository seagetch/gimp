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

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptooloptions.h"
#include "core/gimpdatafactory.h"
#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"

#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpcontainerbox.h"

#include "gimptooloptions-gui.h"
#include "gimpbrushoptions-gui.h"

#include "gimp-intl.h"


#define BRUSH_VIEW_SIZE 256

static void spacing_changed (GimpBrush *brush, gpointer   data);
static void spacing_update (GtkAdjustment *adjustment, gpointer data);
static void update_brush (GtkAdjustment *adjustment, gpointer data);
static void update_brush_shape (GtkWidget *widget, gpointer  *data);
static void notify_brush (GimpBrushGenerated *brush, GParamSpec *pspec, gpointer data);
static void brush_changed (GimpContext *context, GimpData *brush_data, gpointer data);

/*  private functions  */
typedef struct _BrushDialogPrivate BrushDialogPrivate;
struct _BrushDialogPrivate
{
  GHashTable  *adj_hash;
  GtkAdjustment *adj;
  GtkWidget     *shape_group;
  GtkWidget     *preview;
  GimpContainer *container;
  GimpContext   *context;
  gint           spacing_changed_handler_id;
};

static BrushDialogPrivate *
brush_dialog_private_new (void)
{
  BrushDialogPrivate *p = g_new0 (BrushDialogPrivate, 1);
  p->adj_hash = g_hash_table_new (g_str_hash, g_str_equal);
  return p;
}

static void 
brush_dialog_private_set_adjustment (BrushDialogPrivate *p, GtkAdjustment *adj)
{
  p->adj = adj;
}

static void 
brush_dialog_private_set_shape_group (BrushDialogPrivate *p, GtkWidget *shape_group)
{
  p->shape_group = shape_group;
}

static void 
brush_dialog_private_set_preview (BrushDialogPrivate *p, GtkWidget *preview)
{
  p->preview = preview;
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

  p->shape_group = NULL;
  p->adj         = NULL;
  p->preview     = NULL;
  
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
    
  if (p->adj_hash)
    {
      g_hash_table_unref (p->adj_hash);
    }

  if (p)
    g_free (p);
}

static void
brush_changed (GimpContext    *context,
               GimpData       *brush_data,
               gpointer        data)
{
  BrushDialogPrivate      *p            = (BrushDialogPrivate*)data;
  GimpBrush               *brush        = GIMP_BRUSH (brush_data);
  GimpBrushGeneratedShape  shape        = GIMP_BRUSH_GENERATED_CIRCLE;
  gdouble                  radius       = 0.0;
  gint                     spikes       = 2;
  gdouble                  hardness     = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gdouble                  spacing      = 0.0;
  gboolean                 editable;

  if (brush) /* BUG: old brush must be used, but "brush" is new brush object ... */
    g_signal_handlers_disconnect_by_func (brush, notify_brush, p);

  if (brush)
    g_signal_connect (brush, "notify",
                      G_CALLBACK (notify_brush),
                      p);

  if (p->preview)
    gimp_view_set_viewable (GIMP_VIEW (p->preview), GIMP_VIEWABLE (brush_data));

  if (brush_data && GIMP_IS_BRUSH_GENERATED (brush_data))
    {
      GimpBrushGenerated *brush_generated = GIMP_BRUSH_GENERATED (brush);

      shape    = gimp_brush_generated_get_shape        (brush_generated);
      radius   = gimp_brush_generated_get_radius       (brush_generated);
      spikes   = gimp_brush_generated_get_spikes       (brush_generated);
      hardness = gimp_brush_generated_get_hardness     (brush_generated);
      ratio    = gimp_brush_generated_get_aspect_ratio (brush_generated);
      angle    = gimp_brush_generated_get_angle        (brush_generated);
    }

    spacing  = gimp_brush_get_spacing                (GIMP_BRUSH (brush));
/*    
  editable = brush_data && gimp_data_is_writable (brush_data);

  gtk_widget_set_sensitive (brush_editor->options_table,
                            editable);
*/
  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (p->shape_group),
                                   shape);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "radius")), radius);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "spikes")), spikes);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "hardness")), hardness);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "aspect-ratio")), ratio);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "angle")),        angle);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "spacing")),      spacing);
}

static void
spacing_changed (GimpBrush *brush,
                 gpointer   data)
{
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;
  GimpContext         *context;

  context = p->context;
  g_print ("spacing_changed\n");
  
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
  
  g_print ("spacing_update\n");

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
    
      gimp_brush_set_spacing (brush, gtk_adjustment_get_value (adjustment));

      g_signal_handlers_unblock_matched (brush,
                                         G_SIGNAL_MATCH_DETAIL,
                                         0, g_quark_from_static_string ("spacing-changed"), NULL, NULL, NULL);
    }

}

static void
update_brush (GtkAdjustment *adjustment,
              gpointer       data)
{
  gdouble              d_value_brush;
  gdouble              d_value_adj;
  gint                 i_value_brush;
  gint                 i_value_adj;
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;
  GimpBrush           *brush;
  GimpBrushGenerated  *brush_generated;
  gchar               *prop_name;
  GtkAdjustment       *adj;

  g_return_if_fail (G_IS_OBJECT (p->adj));
  g_return_if_fail (G_IS_OBJECT (p->context));

  brush = gimp_context_get_brush (p->context);

  if (! GIMP_IS_BRUSH_GENERATED (brush))
    return;

  brush_generated = GIMP_BRUSH_GENERATED (brush);

  prop_name = (gchar*)g_object_get_data (G_OBJECT (adjustment), "prop_name");
  g_return_if_fail (prop_name != NULL);

  adj = GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, prop_name));
  g_return_if_fail (adj != NULL);
  
  if (strcmp (prop_name , "spikes") == 0)
    {
      g_object_get (G_OBJECT (brush_generated), prop_name, &i_value_brush, NULL);
      i_value_adj = ROUND (gtk_adjustment_get_value (adj));
      if (i_value_brush != i_value_adj)
        {
          g_signal_handlers_block_by_func (brush, notify_brush, p);

          gimp_data_freeze (GIMP_DATA (brush));
          g_object_freeze_notify (G_OBJECT (brush));
          g_object_set (G_OBJECT (brush), prop_name, i_value_adj, NULL);
          g_object_thaw_notify (G_OBJECT (brush));
          gimp_data_thaw (GIMP_DATA (brush));

          g_signal_handlers_unblock_by_func (brush, notify_brush, p);
        }
    }
  else
    {
      g_object_get (G_OBJECT (brush_generated), prop_name, &d_value_brush, NULL);
      d_value_adj = gtk_adjustment_get_value (adj);
      if (d_value_brush != d_value_adj)
        {
          g_signal_handlers_block_by_func (brush, notify_brush, p);

          gimp_data_freeze (GIMP_DATA (brush));
          g_object_freeze_notify (G_OBJECT (brush));
          g_object_set (G_OBJECT (brush), prop_name, d_value_adj, NULL);
          g_object_thaw_notify (G_OBJECT (brush));
          gimp_data_thaw (GIMP_DATA (brush));

          g_signal_handlers_unblock_by_func (brush, notify_brush, p);
        }
    }

}

static void
update_brush_shape (GtkWidget *widget,
                    gpointer  *data)
{
  GimpBrush *brush;
  GimpBrushGenerated *brush_generated = NULL;
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;

  g_return_if_fail (G_IS_OBJECT (p->context));
  brush = gimp_context_get_brush (p->context);

  if (! GIMP_IS_BRUSH_GENERATED (brush))
    return;

  brush_generated = GIMP_BRUSH_GENERATED (brush);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
      GimpBrushGeneratedShape shape;

      shape = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget),
                                                  "gimp-item-data"));

      if (gimp_brush_generated_get_shape (brush_generated) != shape)
        gimp_brush_generated_set_shape (brush_generated, shape);
    }
}

static void
notify_brush (GimpBrushGenerated *brush,
              GParamSpec         *pspec,
              gpointer            data)
{
  GtkAdjustment *adj     = NULL;
  gdouble        d_value = 0.0;
  gint           i_value = 0;
  
  BrushDialogPrivate  *p      = (BrushDialogPrivate*)data;
  
  g_print ("notify_brush: %s\n", pspec->name);

  if (! strcmp (pspec->name, "shape"))
    {
      g_signal_handlers_block_by_func (p->shape_group,
                                       update_brush_shape,
                                       p);

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (p->shape_group),
                                       brush->shape);

      g_signal_handlers_unblock_by_func (p->shape_group,
                                         update_brush_shape,
                                         p);
/*
      adj   = editor->radius_data;
      value = brush->radius;
*/
    }
  else
    {
      adj = g_hash_table_lookup (p->adj_hash, pspec->name);
      g_return_if_fail (adj != NULL);

      if (strcmp (pspec->name , "spikes") == 0)
        {
          g_object_get (G_OBJECT (brush), pspec->name, &i_value, NULL);
          d_value = i_value;
        }
      else
        {
          g_object_get (G_OBJECT (brush), pspec->name, &d_value, NULL);
        }

      g_signal_handlers_block_by_func (adj, update_brush, p);
      gtk_adjustment_set_value (adj, d_value);
      g_signal_handlers_unblock_by_func (adj, update_brush, p);
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
  
/*
  if (p->adj)
    {
      g_object_unref (G_OBJECT (p->adj));
      p->adj = NULL;
    }

  if (p->shape_group)
    {
      g_object_unref (G_OBJECT (p->shape_group));
      p->shape_group = NULL;
    }
*/
  p->shape_group = NULL;
  p->adj         = NULL;
  p->preview     = NULL;
  
  if (p->container)
    {
      g_object_unref (G_OBJECT (p->container));
      p->container = NULL;
    }

  if (p->context)
    {
      GimpBrush *brush = gimp_context_get_brush (p->context);
      if (brush) /* BUG: old brush must be used, but "brush" is new brush object ... */
        g_signal_handlers_disconnect_by_func (brush, notify_brush, p);
      g_signal_handlers_disconnect_by_func (p->context,
                                            G_CALLBACK (brush_changed), p);
      g_object_unref (G_OBJECT (p->context));
      p->context = NULL;
    }

  g_hash_table_remove_all (p->adj_hash);
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
  GtkWidget                     *vbox;
  GtkWidget                     *frame;
  GtkWidget                     *box;
  GtkWidget                     *table;
  GtkWidget                     *preview;
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

  gtk_box_pack_start (GTK_BOX (*result), GTK_WIDGET (editor), TRUE, TRUE, 0);      
  gtk_widget_show (GTK_WIDGET (editor));
  
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_box_pack_start (GTK_BOX (*result), vbox, TRUE, TRUE, 0);      
  gtk_widget_show (vbox);
  
  table = gtk_table_new (6, 3, FALSE);
  gtk_box_pack_start (GTK_BOX (vbox), table, TRUE, TRUE, 0);      
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
                                     FALSE, 0.0, 0.0, TRUE, FALSE);
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

  g_object_set_data (G_OBJECT (adj), "prop_name", "spacing");
  g_hash_table_replace (p->adj_hash, "spacing", adj);
  brush_dialog_private_set_adjustment (p, adj);
  brush_dialog_private_set_container  (p, container);
  brush_dialog_private_set_context    (p, context); 

  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (spacing_update),
                    p);
  gimp_tool_options_table_increment_next (&inc);  

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  preview = gimp_view_new_full_by_types (context,
                                         GIMP_TYPE_VIEW,
                                         GIMP_TYPE_BRUSH,
                                         BRUSH_VIEW_SIZE,
                                         BRUSH_VIEW_SIZE, 0,
                                         FALSE, FALSE, TRUE);
  brush_dialog_private_set_preview (p, preview);
  gtk_widget_set_size_request (preview, -1, BRUSH_VIEW_SIZE);
  gimp_view_set_expand (GIMP_VIEW (preview), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), preview);
  gtk_widget_show (preview);

  p->shape_group = NULL;

  /* Stock Box for the brush shape */
  box = gimp_enum_stock_box_new (GIMP_TYPE_BRUSH_GENERATED_SHAPE,
                                 "gimp-shape",
                                 GTK_ICON_SIZE_MENU,
                                 G_CALLBACK (update_brush_shape),
                                 p,
                                 &p->shape_group);
                                 
  gimp_table_attach_aligned (GTK_TABLE (table),
                             gimp_tool_options_table_increment_get_col (&inc),
                             gimp_tool_options_table_increment_get_row (&inc),
                             _("Shape:"), 0.0, 0.5,
                             box, 2, TRUE);
  gtk_widget_show (box);
  gimp_tool_options_table_increment_next (&inc);  

  /*  brush radius scale  */
  adj =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                          gimp_tool_options_table_increment_get_col (&inc),
                                          gimp_tool_options_table_increment_get_row (&inc),
                                          _("Radius:"), -1, 5,
                                          0.0, 0.1, 1000.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  gimp_scale_entry_set_logarithmic (GTK_OBJECT (adj), TRUE);

  g_object_set_data (G_OBJECT (adj), "prop_name", "radius");
  g_hash_table_replace (p->adj_hash, "radius", adj);
  gimp_tool_options_table_increment_next (&inc);  
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (update_brush),
                    p);

  /*  number of spikes  */
  adj =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                          gimp_tool_options_table_increment_get_col (&inc),
                                          gimp_tool_options_table_increment_get_row (&inc),
                                          _("Spikes:"), -1, 5,
                                          2.0, 2.0, 20.0, 1.0, 1.0, 0,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_object_set_data (G_OBJECT (adj), "prop_name", "spikes");
  g_hash_table_replace (p->adj_hash, "spikes", adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (update_brush),
                    p);
  gimp_tool_options_table_increment_next (&inc);  

  /*  brush hardness scale  */
  adj =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                          gimp_tool_options_table_increment_get_col (&inc),
                                          gimp_tool_options_table_increment_get_row (&inc),
                                          _("Hardness:"), -1, 5,
                                          0.0, 0.0, 1.0, 0.01, 0.1, 2,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_object_set_data (G_OBJECT (adj), "prop_name", "hardness");
  g_hash_table_replace (p->adj_hash, "hardness", adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (update_brush),
                    p);
  gimp_tool_options_table_increment_next (&inc);  

  /*  brush aspect ratio scale  */
  adj =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                          gimp_tool_options_table_increment_get_col (&inc),
                                          gimp_tool_options_table_increment_get_row (&inc),
                                          _("Aspect ratio:"), -1, 5,
                                          0.0, 1.0, 20.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_object_set_data (G_OBJECT (adj), "prop_name", "aspect-ratio");
  g_hash_table_replace (p->adj_hash, "aspect-ratio", adj);
  g_signal_connect (adj,"value-changed",
                    G_CALLBACK (update_brush),
                    p);
  gimp_tool_options_table_increment_next (&inc);  

  /*  brush angle scale  */
  adj =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (table),
                                          gimp_tool_options_table_increment_get_col (&inc),
                                          gimp_tool_options_table_increment_get_row (&inc),
                                          _("Angle:"), -1, 5,
                                          0.0, 0.0, 180.0, 0.1, 1.0, 1,
                                          TRUE, 0.0, 0.0,
                                          NULL, NULL));

  g_object_set_data (G_OBJECT (adj), "prop_name", "angle");
  g_hash_table_replace (p->adj_hash, "angle", adj);
  g_signal_connect (adj, "value-changed",
                    G_CALLBACK (update_brush),
                    p);
  gimp_tool_options_table_increment_next (&inc);  

#if 0
  /*  brush spacing  */
  editor->spacing_data =
    GTK_ADJUSTMENT (gimp_scale_entry_new (GTK_TABLE (editor->options_table),
                                          0, row++,
                                          _("Spacing:"), -1, 5,
                                          0.0, 1.0, 200.0, 1.0, 10.0, 1,
                                          FALSE, 1.0, 5000.0,
                                          _("Percentage of width of brush"),
                                          NULL));

  g_signal_connect (editor->spacing_data, "value-changed",
                    G_CALLBACK (gimp_brush_editor_update_brush),
                    editor);
#endif                    
  p->spacing_changed_handler_id =
    gimp_container_add_handler (container, "spacing-changed",
                                G_CALLBACK (spacing_changed),
                                p);

  gimp_view_set_viewable (GIMP_VIEW (preview), GIMP_VIEWABLE (brush));

  g_signal_connect (context,
                    gimp_context_type_to_signal_name (GIMP_TYPE_BRUSH),
                    G_CALLBACK (brush_changed),
                    p);
//  g_signal_connect (brush, "notify", G_CALLBACK (notify_brush), p);
  if (context && brush)
    brush_changed (context, GIMP_DATA(brush), p);

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
