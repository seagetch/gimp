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

#include <glib-object.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "tools-types.h"

#include "base/temp-buf.h"

#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptooloptions.h"
#include "core/gimpdatafactory.h"
#include "core/gimpmypaintbrush.h"

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

};
#include "gimptooloptions-gui-cxx.hpp"
#include "base/delegators.hpp"



class MypaintPopupPrivate {
  static const int MYPAINT_BRUSH_VIEW_SIZE = 256;

  GimpContainer *container;
  GimpContext   *context;
  gint           spacing_changed_handler_id;
  
public:
  MypaintPopupPrivate(GimpContainer* ctn, GimpContext* ctx) {
    container = ctn;
    context   = ctx;
  }
  
  ~MypaintPopupPrivate();
  void create(GObject  *button,
              GtkWidget **result);
  void destroy(GObject* object);
  void update_brush (GObject *adjustment);
  void notify_brush (GObject *brush, GParamSpec *pspec);
  void brush_changed (GObject *object, GimpData *brush_data);

};


void
MypaintPopupPrivate::update_brush (GObject* object)
{
  GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
  gdouble              d_value_brush;
  gdouble              d_value_adj;
  gint                 i_value_brush;
  gint                 i_value_adj;
  GimpMypaintBrush    *brush;
  gchar               *prop_name;
  GtkAdjustment       *adj;

  g_return_if_fail (G_IS_OBJECT (adj));
  g_return_if_fail (G_IS_OBJECT (context));

  brush = gimp_context_get_mypaint_brush (context);

}

void
MypaintPopupPrivate::notify_brush (GObject* object,
                                   GParamSpec         *pspec)
{
  GimpMypaintBrush *brush = GIMP_MYPAINT_BRUSH (object);
  GtkAdjustment *adj     = NULL;
  gdouble        d_value = 0.0;
  gint           i_value = 0;
  
  g_print ("notify_brush: %s\n", pspec->name);

  if (! strcmp (pspec->name, "shape"))
    {
    /*
      g_signal_handlers_block_by_func (p->shape_group,
                                       update_brush_shape,
                                       p);

      gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (p->shape_group),
                                       brush->shape);

      g_signal_handlers_unblock_by_func (p->shape_group,
                                         update_brush_shape,
                                         p);
    */
/*
      adj   = editor->radius_data;
      value = brush->radius;
*/
    }
  else
    {
    /*
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
    */
    }
}

void
MypaintPopupPrivate::brush_changed (GObject*  object,
                                    GimpData* brush_data)
{
  GimpMypaintBrush        *brush        = GIMP_MYPAINT_BRUSH (brush_data);
  gdouble                  radius       = 0.0;
  gdouble                  ratio        = 0.0;
  gdouble                  angle        = 0.0;
  gboolean                 editable     = false;

  g_print("brush changed\n");

/*
  if (brush) // BUG: old brush must be used, but "brush" is new brush object ...
    g_signal_handlers_disconnect_by_func (brush, notify_brush, p);
*/

/*
  if (brush)
    g_signal_connect (brush, "notify",
                      G_CALLBACK (notify_brush),
                      p);
*/
/*    
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

  editable = brush_data && gimp_data_is_writable (brush_data);
  gtk_widget_set_sensitive (p->brush_frame, editable);

  gimp_int_radio_group_set_active (GTK_RADIO_BUTTON (p->shape_group),
                                   shape);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "radius")), radius);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "spikes")), spikes);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "hardness")), hardness);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "aspect-ratio")), ratio);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "angle")),        angle);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (g_hash_table_lookup (p->adj_hash, "spacing")),      spacing);

  if (brush && context)
    {
      gdouble value = MAX (brush->mask->width, brush->mask->height);
      g_object_set (context, "brush-size", value, NULL);
    }
*/
}

MypaintPopupPrivate::~MypaintPopupPrivate ()
{
}

void
MypaintPopupPrivate::destroy (GObject* object)
{
/*
  if (spacing_changed_handler_id)
    {
      gimp_container_remove_handler (container,
                                     spacing_changed_handler_id);
      pspacing_changed_handler_id = 0;
    }
*/  
#if 0  
  if (container)
    {
      g_object_unref (G_OBJECT (container));
      container = NULL;
    }

  if (context)
    {
      GimpBrush *brush = gimp_context_get_brush (context);
      /*
      if (brush) // BUG: old brush must be used, but "brush" is new brush object ...
        g_signal_handlers_disconnect_by_func (brush, notify_brush, p);
      g_signal_handlers_disconnect_by_func (p->context,
                                            G_CALLBACK (brush_changed), p);
      */
      g_object_unref (G_OBJECT (context));
      context = NULL;
    }
#endif
}

void
MypaintPopupPrivate::create (GObject* object,
                             GtkWidget **result)
{
  GtkButton*                     button = GTK_BUTTON(object);
  GimpContainerEditor           *editor;
  GimpMypaintBrush              *brush;
  GtkWidget                     *vbox;
  GtkWidget                     *frame;
  GtkWidget                     *box;
  GtkWidget                     *table;
  GtkWidget                     *frame2;
  GimpViewType                   view_type = GIMP_VIEW_TYPE_GRID;
  GimpViewSize                   view_size = GIMP_VIEW_SIZE_LARGE;
  gint                           view_border_width = 1;
  gint                           default_view_size = GIMP_VIEW_SIZE_LARGE;
  GimpToolOptionsTableIncrement  inc = gimp_tool_options_table_increment (FALSE);
  GList                         *children;
  GtkAdjustment                 *adj = NULL;
  
  container = gimp_data_factory_get_container (context->gimp->mypaint_brush_factory);
  brush     = gimp_context_get_mypaint_brush (context);
  
  g_return_if_fail (GIMP_IS_CONTAINER (container));
  g_return_if_fail (GIMP_IS_CONTEXT (context));
  g_return_if_fail (view_size >  0 &&
                    view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE);
  g_return_if_fail (view_border_width >= 0 &&
                    view_border_width <= GIMP_VIEW_MAX_BORDER_WIDTH);

  *result    = gtk_hbox_new (FALSE, 1);
  gtk_widget_show (*result);
  
  editor = GIMP_CONTAINER_EDITOR (
    g_object_new (GIMP_TYPE_CONTAINER_EDITOR,
      "view-type", view_type,
      "container", container,
      "context",   context,
      "view-size", view_size,
      "view-border-width", view_border_width,
      NULL));
  gimp_container_view_set_reorderable (GIMP_CONTAINER_VIEW (editor->view),
                                       FALSE);

  gimp_container_box_set_size_request (GIMP_CONTAINER_BOX (editor->view),
                                       6  * (default_view_size +
                                             2 * view_border_width),
                                       10 * (default_view_size +
                                             2 * view_border_width));

  gtk_box_pack_start (GTK_BOX (*result), GTK_WIDGET (editor), TRUE, TRUE, 0);      
  gtk_widget_show (GTK_WIDGET (editor));
  

  g_signal_connect_delegator (G_OBJECT(context),
                              gimp_context_type_to_signal_name (GIMP_TYPE_MYPAINT_BRUSH),
                              Delegator::delegator(this, &MypaintPopupPrivate::brush_changed));

/*
//  g_signal_connect (brush, "notify", G_CALLBACK (notify_brush), p);
*/
  if (context && brush)
    brush_changed (G_OBJECT(context), GIMP_DATA(brush));
  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, FALSE);
  
  g_signal_connect_delegator (G_OBJECT (*result), "destroy", 
                              Delegator::delegator(this, &MypaintPopupPrivate::destroy));
}

extern "C" {
/*  public functions  */
GtkWidget*
gimp_mypaint_brush_button_with_popup (GObject *config)
{
  GimpContainer *container;
  GimpContext   *context;
  GimpViewSize   view_size = GIMP_VIEW_SIZE_SMALL;
  const gchar        *prop_name;
  GtkWidget     *label_widget;

  context   = GIMP_CONTEXT (config);
  container = gimp_data_factory_get_container (context->gimp->mypaint_brush_factory);
  
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE, NULL);

  prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

  label_widget = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                     context, view_size);
  gtk_widget_show (label_widget);
  
  MypaintPopupPrivate* priv = new MypaintPopupPrivate(container, context);
  
  return gimp_tool_options_button_with_popup (label_widget,
                                               Delegator::delegator(priv, &MypaintPopupPrivate::create),
                                               priv);
}
};
