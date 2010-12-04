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

#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "core/gimpcontext.h"
#include "core/gimptooloptions.h"
#include "core/gimpdatafactory.h"
#include "core/gimpdynamics.h"

#include "core/gimpbrush.h"
#include "core/gimpbrushgenerated.h"

#include "widgets/gimpview.h"
#include "widgets/gimpviewrenderer.h"
#include "widgets/gimppropwidgets.h"
#include "widgets/gimpcontainereditor.h"
#include "widgets/gimpcontainerview.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpcontainerbox.h"
#include "widgets/gimpdynamicsoutputeditor.h"

#include "gimptooloptions-gui.h"
#include "gimpdynamicsoptions-gui.h"

#include "gimp-intl.h"


#define BRUSH_VIEW_SIZE 256

static void notify_working_dynamics (GimpDynamics *dynamics, GParamSpec *pspec, gpointer data);
static void notify_current_target_dynamics (GimpDynamics *dynamics, GParamSpec *pspec, gpointer data);
static void dynamics_changed (GimpContext *context, GimpData *brush_data, gpointer data);

/*  private functions  */
typedef struct _DynamicsDialogPrivate DynamicsDialogPrivate;
struct _DynamicsDialogPrivate
{
  GHashTable  *adj_hash;
  GimpDynamics *working_dynamics;
  GimpDynamics *current_target_dynamics;
};

static DynamicsDialogPrivate *
dynamics_dialog_private_new (void)
{
  DynamicsDialogPrivate *p = g_new0 (DynamicsDialogPrivate, 1);
  p->adj_hash = g_hash_table_new (g_str_hash, g_str_equal);
  p->working_dynamics = g_object_new (GIMP_TYPE_DYNAMICS, NULL);
  return p;
}

static void
dynamics_dialog_private_destroy (gpointer data)
{
  DynamicsDialogPrivate *p = (DynamicsDialogPrivate*)data;
  
  g_print ("DynamicsDialogPrivate::destroy\n");

  if (p->adj_hash)
    {
      g_hash_table_unref (p->adj_hash);
    }
    
  if (p->working_dynamics)
    {
      g_object_unref (G_OBJECT (p->working_dynamics));
    }
  p->current_target_dynamics = NULL;

  if (p)
    g_free (p);
}

static void
dynamics_changed (GimpContext    *context,
                  GimpData       *dynamics_data,
                  gpointer        data)
{
  DynamicsDialogPrivate *p        = (DynamicsDialogPrivate*)data;
  GimpDynamics          *dynamics = GIMP_DYNAMICS (dynamics_data);

  g_return_if_fail (p && context);

  if (p->current_target_dynamics)
    g_signal_handlers_disconnect_by_func (p->current_target_dynamics,
                                          notify_current_target_dynamics,
                                          p);
  p->current_target_dynamics = dynamics;
                                        
  g_return_if_fail (dynamics);
  
  notify_current_target_dynamics (dynamics, NULL, p);

  g_signal_connect (p->current_target_dynamics, "notify",
                    G_CALLBACK (notify_current_target_dynamics),
                    p);
/*
  gtk_widget_set_sensitive (dynamics_editor->check_grid,
                            editor->data_editable);
*/
}

static void
notify_current_target_dynamics (GimpDynamics *dynamics,
                                GParamSpec   *pspec,
                                gpointer      data)
{
  DynamicsDialogPrivate *p        = (DynamicsDialogPrivate*)data;

  g_print ("notify::current_target_dynamics\n");
  
  g_return_if_fail (p && dynamics == p->current_target_dynamics);

  g_signal_handlers_block_by_func (p->working_dynamics,
                                   notify_working_dynamics,
                                   p);

  gimp_config_copy (GIMP_CONFIG (p->current_target_dynamics),
                    GIMP_CONFIG (p->working_dynamics),
                    GIMP_CONFIG_PARAM_SERIALIZE);

  g_signal_handlers_unblock_by_func (p->working_dynamics,
                                     notify_working_dynamics,
                                     p);
}

static void
notify_working_dynamics        (GimpDynamics *dynamics,
                                GParamSpec   *pspec,
                                gpointer      data)
{
  DynamicsDialogPrivate *p        = (DynamicsDialogPrivate*)data;
  
  g_print ("notify::working_dynamics\n");
  
  g_return_if_fail (p && dynamics == p->working_dynamics);

  if (p->current_target_dynamics)
    {
      g_signal_handlers_block_by_func (p->current_target_dynamics,
                                       notify_current_target_dynamics,
                                       p);

      gimp_config_copy (GIMP_CONFIG (p->working_dynamics),
                        GIMP_CONFIG (p->current_target_dynamics),
                        GIMP_CONFIG_PARAM_SERIALIZE);

      g_signal_handlers_unblock_by_func (p->current_target_dynamics,
                                         notify_current_target_dynamics,
                                         p);
    }
}

static void
destroy_dynamics_popup_dialog (GtkWidget *widget, gpointer data)
{
  DynamicsDialogPrivate *p = (DynamicsDialogPrivate*)data;
  
  g_print ("popup destroyed\n");

  if (p)
    {
      if (p->current_target_dynamics)
        {
          g_signal_handlers_disconnect_by_func (p->current_target_dynamics,
                                                notify_current_target_dynamics,
                                                p);
          p->current_target_dynamics = NULL;
        }
      g_signal_handlers_disconnect_by_func (p->working_dynamics,
                                            notify_working_dynamics,
                                            p);
    }

  g_hash_table_remove_all (p->adj_hash);
}

static void
create_dynamics_popup_dialog (GtkWidget  *button,
                           GtkWidget **result,
                           GObject    *config,
                           gpointer    data)
{
  GimpContainer                 *container;
  GimpContext                   *context;
  GimpContainerEditor           *editor;
  GimpDynamics                  *dynamics;
  GtkWidget                     *vbox;
  GtkWidget                     *table;
  GType                          enum_type;
  GEnumClass                    *enum_class;
  GEnumValue                    *value;
    
  GimpViewType                   view_type = GIMP_VIEW_TYPE_LIST;
  GimpViewSize                   view_size = GIMP_VIEW_SIZE_SMALL;
  gint                           view_border_width = 1;
  gint                           default_view_size = GIMP_VIEW_SIZE_SMALL;
 
  GtkWidget                     *notebook;

  GList                         *children;
  DynamicsDialogPrivate         *p = (DynamicsDialogPrivate*)data;


  GtkAdjustment                 *adj = NULL;
  GtkWidget                     *preview; 
  GimpToolOptionsTableIncrement  inc = gimp_tool_options_table_increment (FALSE);
  GtkWidget                     *frame;
  GtkWidget                     *box;
  
  context   = GIMP_CONTEXT (config);
  container = gimp_data_factory_get_container (context->gimp->dynamics_factory);
  
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

  /* Dynamics editor */
  notebook = gtk_notebook_new ();
  gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
  gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
  gtk_widget_show (notebook);

  /* Dynamics output editor */
  enum_type = GIMP_TYPE_DYNAMICS_OUTPUT_TYPE;
  enum_class = g_type_class_ref (enum_type);

  for (value = enum_class->values;
       value->value_name;
       value++)
    {
      GimpDynamicsOutput *output;
      GtkWidget          *output_editor;

      if (value->value < enum_class->minimum || value->value > enum_class->maximum)
        continue;

      output = gimp_dynamics_get_output (p->working_dynamics, value->value);
      output_editor = gimp_dynamics_output_editor_new (output);

      gtk_notebook_append_page (GTK_NOTEBOOK (notebook), output_editor, gtk_label_new (_(value->value_nick)));
      gtk_widget_show (output_editor);
    }

  g_type_class_unref (enum_class);

  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, FALSE);

  g_signal_connect (context,
                    gimp_context_type_to_signal_name (GIMP_TYPE_DYNAMICS),
                    G_CALLBACK (dynamics_changed),
                    p);
  g_signal_connect (p->working_dynamics,
                    "notify", G_CALLBACK (notify_working_dynamics), p);
  g_signal_connect (GTK_WIDGET (*result), "destroy", G_CALLBACK(destroy_dynamics_popup_dialog), p);
}


/*  public functions  */
GtkWidget*
gimp_dynamics_button_with_popup (GObject *config)
{
  GimpContainer *container;
  GimpContext   *context;
  GimpViewSize   view_size = GIMP_VIEW_SIZE_SMALL;
  const gchar  *prop_name;
  GtkWidget     *label_widget;
  DynamicsDialogPrivate *p = NULL;

  context   = GIMP_CONTEXT (config);
  container = gimp_data_factory_get_container (context->gimp->dynamics_factory);
  
  g_return_val_if_fail (GIMP_IS_CONTAINER (container), NULL);
  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size >  0 &&
                        view_size <= GIMP_VIEWABLE_MAX_BUTTON_SIZE, NULL);

  prop_name = gimp_context_type_to_prop_name (gimp_container_get_children_type (container));

  label_widget = gimp_prop_view_new (G_OBJECT (context), prop_name,
                                     context, view_size);
  gtk_widget_show (label_widget);
  
  p = dynamics_dialog_private_new ();

  return gimp_tool_options_button_with_popup (config, label_widget,
                                               create_dynamics_popup_dialog,
                                               p, dynamics_dialog_private_destroy);
}
