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

#include "core/gimptooloptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimppopupbutton.h"

#include "gimptooloptions-gui.h"


/*  public functions  */
GtkWidget *
gimp_tool_options_gui (GimpToolOptions *tool_options)
{
  return gimp_tool_options_gui_full (tool_options, FALSE);
}

GtkWidget *
gimp_tool_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal)
{
  GtkWidget *vbox;

  g_return_val_if_fail (GIMP_IS_TOOL_OPTIONS (tool_options), NULL);

  if (horizontal)
    {
      vbox = gtk_hbox_new (FALSE, 6);
    }
  else
    {
      vbox = gtk_vbox_new (FALSE, 6);
    }

  return vbox;
}

GtkWidget * gimp_tool_options_table (gint num_items, gboolean horizontal)
{
  GtkWidget *table;
  
  g_return_val_if_fail (num_items > 0, NULL);
  
  if (horizontal)
    {
      table = gtk_table_new (1, num_items * 3, FALSE);
    }
  else
    {
      table = gtk_table_new (num_items, 3, FALSE);
    }
  
  return table;
}

GimpToolOptionsTableIncrement gimp_tool_options_table_increment (gboolean horizontal)
{
  GimpToolOptionsTableIncrement result;
  result.table_col = result.table_row = 0;
  result.horizontal = horizontal;
  return result;
}

inline 
gint gimp_tool_options_table_increment_get_col (GimpToolOptionsTableIncrement* inc)
{
  return inc->table_col;
}

inline 
gint gimp_tool_options_table_increment_get_row (GimpToolOptionsTableIncrement* inc)
{
  return inc->table_row;
}

inline
void gimp_tool_options_table_increment_next (GimpToolOptionsTableIncrement* inc)
{
  if (inc->horizontal)
    inc->table_col += 3;
  else
    inc->table_row ++;
}

void
gimp_tool_options_setup_popup_layout (GList *children, gboolean hide_label)
{
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (hide_label && GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
      children = next;
    }

}


typedef struct _GimpCreateViewInternal GimpCreateViewInternal;
typedef struct _GimpCreateScaleEntry   GimpCreateScaleEntry;
struct _GimpCreateScaleEntry
{
  gchar  *property_name;
  gchar  *text;
  gdouble       step_increment;
  gdouble       page_increment;
  gint          digits;
  gboolean      limit_scale;
  gdouble       lower_limit;
  gdouble       upper_limit;
  gboolean      logarithm;
  gboolean      opacity;
};

struct _GimpCreateViewInternal 
{
  GObject                        *config;
  GimpPopupCreateViewForTheTable  create_view;
  gboolean                        erase_label;
  gpointer                        data;
  void  (*destroy_data) (gpointer data);
};

static void
scale_entry_destroy (gpointer data)
{
  GimpCreateScaleEntry *entry_state = (GimpCreateScaleEntry*)data;
  g_free (entry_state->property_name);
  g_free (entry_state->text);
}

static void
create_view_destroy (gpointer data, GClosure *closure)
{
  GimpCreateViewInternal *state = (GimpCreateViewInternal*)data;
  if (!state)
    return;

  if (state->config)
    {
      g_object_unref (state->config);
      state->config = NULL;
    }
  if (state->data && state->destroy_data)
    {
      (*state->destroy_data) (state->data);
      state->data = NULL;
    }
  g_free (state);
}

static void
create_view_for_inner_table_create_view (GtkWidget *button, GtkWidget **result, GimpCreateViewInternal *state)
{
  GtkWidget *table;
  GList     *children;

  g_return_if_fail (state != NULL);
  g_return_if_fail (G_IS_OBJECT (state->config));
  g_return_if_fail (state->create_view != NULL);
  
  table = gtk_table_new (1, 3, FALSE);
  (*state->create_view)(table, 0, 0, state->config);

  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, state->erase_label);

  *result = table;
  
}

static void
create_view_for_scale_entry (GtkWidget *button, GtkWidget **result, GimpCreateViewInternal *state)
{
  GtkWidget *table;
  GimpCreateScaleEntry *entry_state = (GimpCreateScaleEntry*)(state->data);
  GList     *children;

  g_return_if_fail (state != NULL);
  g_return_if_fail (G_IS_OBJECT (state->config));
  g_return_if_fail (entry_state != NULL);
  
  table = gtk_table_new (1, 3, FALSE);
  if (entry_state->opacity)
    {
      GtkObject *adj;
      adj   = gimp_prop_opacity_entry_new (state->config, entry_state->property_name,
                                           GTK_TABLE (table), 0, 0,
                                           entry_state->text);
    }
  else
    {
      GtkObject *adj;
      adj = gimp_prop_scale_entry_new (state->config, entry_state->property_name,
                                       GTK_TABLE (table), 
                                       0, 0,
                                       entry_state->text,
                                       entry_state->step_increment, 
                                       entry_state->page_increment,
                                       entry_state->digits,
                                       entry_state->limit_scale, 
                                       entry_state->lower_limit, 
                                       entry_state->upper_limit);
      if (entry_state->logarithm)
        gimp_scale_entry_set_logarithmic (GTK_OBJECT (adj), TRUE);    
    }

  children = gtk_container_get_children (GTK_CONTAINER (table));  
  gimp_tool_options_setup_popup_layout (children, state->erase_label);

  *result = table;
}

static GClosure *
generate_create_view_closure (GObject *config, 
                              GimpPopupCreateViewForTheTable create_view,
                              gboolean erase_label)
{
  GimpCreateViewInternal *state = g_new0 (GimpCreateViewInternal, 1);
  state->config = config;
  g_object_ref (config);
  state->create_view = create_view;
  state->erase_label = erase_label;
  state->data        = NULL;
  state->destroy_data = NULL;
  
  return g_cclosure_new (G_CALLBACK (create_view_for_inner_table_create_view),
                         state, create_view_destroy);
}

static GClosure *
generate_create_scale_entry_closure (GObject *config, 
                                     const gchar  *property_name,
                                     const gchar  *text,
                                     gdouble       step_increment,
                                     gdouble       page_increment,
                                     gint          digits,
                                     gboolean      limit_scale,
                                     gdouble       lower_limit,
                                     gdouble       upper_limit,
                                     gboolean      logarithm,
                                     gboolean      opacity,
                                     gboolean erase_label)
{
  GimpCreateViewInternal *state = g_new0 (GimpCreateViewInternal, 1);
  GimpCreateScaleEntry* entry_state = g_new0 (GimpCreateScaleEntry, 1);
  entry_state->property_name  = g_strdup (property_name);
  entry_state->text           = g_strdup (text);
  entry_state->step_increment = step_increment;
  entry_state->page_increment = page_increment;
  entry_state->digits         = digits;
  entry_state->limit_scale    = limit_scale;
  entry_state->lower_limit    = lower_limit;
  entry_state->upper_limit    = upper_limit;
  entry_state->logarithm      = logarithm;
  entry_state->opacity        = opacity;

  state->data = entry_state;
  state->config = config;
  g_object_ref (config);

  state->erase_label = TRUE;
  state->destroy_data = scale_entry_destroy;
  
  return g_cclosure_new (G_CALLBACK (create_view_for_scale_entry),
                         state, create_view_destroy);
}

GtkWidget *
gimp_tool_options_frame_gui_with_popup (GObject                    *config,
                                        GType                       tool_type, 
                                        gchar                      *label,
                                        gboolean                    horizontal,
                                        GimpPopupCreateViewCallback create_view)
{
  GtkWidget *result = NULL;

  if (horizontal)
    {
      GtkWidget  *button_label;
      GtkWidget  *button;
      GtkBox     *hbox;
      
      hbox          = GTK_BOX (gtk_hbox_new (FALSE, 0));
      button_label  = gtk_label_new (label);
      gtk_box_pack_start (GTK_BOX (hbox), button_label, TRUE, TRUE, 0);      
      gtk_widget_show (button_label);
      button_label  = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_IN);
      gtk_box_pack_end (GTK_BOX (hbox), button_label, FALSE, FALSE, 0);
      gtk_widget_show (button_label);
      gtk_widget_show (GTK_WIDGET (hbox));

      button        = gimp_popup_button_new (GTK_WIDGET (hbox));
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_widget_show (button);
      
      g_signal_connect_object (button, "create-view", G_CALLBACK (create_view), config, 0);
      
      result = GTK_WIDGET (button);
    }
  else
    {
      GtkWidget *table = NULL;
      result = gimp_frame_new (label);
      if (create_view)
        {
          (*create_view)(NULL, &table, config);
          gtk_container_add (GTK_CONTAINER (result), table);
          gtk_widget_show (table);
        }
      gtk_widget_show (result);
    }

  return result;
}

GtkWidget *
gimp_tool_options_toggle_gui_with_popup (GObject           *config,
                                         GType                       tool_type, 
                                         gchar                      *property_name,
                                         gchar                      *short_label,
                                         gchar                      *long_label,
                                         gboolean                    horizontal,
                                         GimpPopupCreateViewCallback create_view)
{
  GtkWidget *result;

  if (horizontal)
    {
      GtkWidget  *button_label;
      GtkWidget  *button;
      GtkBox     *hbox;
      
      hbox          = GTK_BOX (gtk_hbox_new (FALSE, 0));
      button_label  = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_ETCHED_IN);
      
      button        = gimp_prop_check_button_new (config, property_name, short_label);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      button        = gimp_popup_button_new (button_label);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_box_pack_start (hbox, button, FALSE, TRUE, 0);
      gtk_widget_show (button);
      
      g_signal_connect_object (button, "create-view", G_CALLBACK (create_view), config, 0);
      
      result = GTK_WIDGET (hbox);
    }
  else
    {
      GtkWidget *frame;
      GtkWidget *table;
      if (create_view)
        (*create_view)(NULL, &table, config);

      frame = gimp_prop_expanding_frame_new (config, property_name,
                                             long_label,
                                             table, NULL);
      result = frame;
    }

  return result;
}

static void
config_notify_label (GObject *config, 
                     GParamSpec *param_spec,
                     GtkWidget  *label)
{
  gdouble value;
  gchar str[40];
  gint digits = 1;

  if (G_IS_PARAM_SPEC_INT (param_spec))
    {
      gint int_value;

      g_object_get (config, param_spec->name, &int_value, NULL);

      value = int_value;
    }
  else if (G_IS_PARAM_SPEC_UINT (param_spec))
    {
      guint uint_value;

      g_object_get (config, param_spec->name, &uint_value, NULL);

      value = uint_value;
    }
  else if (G_IS_PARAM_SPEC_LONG (param_spec))
    {
      glong long_value;

      g_object_get (config, param_spec->name, &long_value, NULL);

      value = long_value;
    }
  else if (G_IS_PARAM_SPEC_ULONG (param_spec))
    {
      gulong ulong_value;

      g_object_get (config, param_spec->name, &ulong_value, NULL);

      value = ulong_value;
    }
  else if (G_IS_PARAM_SPEC_INT64 (param_spec))
    {
      gint64 int64_value;

      g_object_get (config, param_spec->name, &int64_value, NULL);

      value = int64_value;
    }
  else if (G_IS_PARAM_SPEC_UINT64 (param_spec))
    {
      guint64 uint64_value;

      g_object_get (config, param_spec->name, &uint64_value, NULL);

#if defined _MSC_VER && (_MSC_VER < 1300)
      value = (gint64) uint64_value;
#else
      value = uint64_value;
#endif
    }
  else if (G_IS_PARAM_SPEC_DOUBLE (param_spec))
    {
      g_object_get (config, param_spec->name, &value, NULL);

      if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label),
                                              "percentage")))
        value *= 100.0;

    }
  else
    {
      g_warning ("%s: unhandled param spec of type %s",
                 G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (param_spec));
      return;
    }
    
    digits = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (label), "digits"));
    g_sprintf (str, "%1.*f", digits, value);
    gtk_label_set_text (GTK_LABEL (label), str);
}

static void
gimp_tool_options_scale_entry_new_internal (GObject      *config,
                                            const gchar *property_name,
                                            GtkTable     *table,
                                            gint          column,
                                            gint          row,
                                            const gchar *text,
                                            gdouble       step_increment,
                                            gdouble       page_increment,
                                            gint          digits,
                                            gboolean      limit_scale,
                                            gdouble       lower_limit,
                                            gdouble       upper_limit,
                                            gboolean      logarithm,
                                            gboolean      opacity,
                                            gboolean      horizontal)
/*                                          
                                   const gchar *property_name,
                                   GtkTable    *table,
                                   gint         column,
                                   gint         row,
                                   gchar       *text,
                                   gint         digits,
                                   gboolean     percentage,
                                   gboolean     horizontal,
                                   GimpPopupCreateViewForTheTable create_view)
*/
{
  if (horizontal)
    {
      gchar      *signal_detail;
      GtkWidget  *button_label;
      GtkWidget  *label;
      GtkWidget  *button;
      GParamSpec *param_spec;
      GClosure   *closure;
      
      button_label  = gtk_label_new ("");
      button        = gimp_popup_button_new (button_label);
      param_spec    = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                    property_name);
      label         = gtk_label_new (text);

      if (opacity)
        g_object_set_data (G_OBJECT (button_label), "percentage", GINT_TO_POINTER (opacity));

      g_object_set_data (G_OBJECT (button_label), "digits", GINT_TO_POINTER (digits));
      
      if (param_spec)
        config_notify_label (config, param_spec, button_label);

      gtk_widget_show (button);
      gtk_widget_show (label);
      
      gtk_table_attach (GTK_TABLE (table), label,
	                      column    , column + 1, row, row + 1,
	                      GTK_FILL, GTK_FILL, 0, 0);
	    
      gtk_table_attach (GTK_TABLE (table), button,
	                      column + 1, column + 3, row, row + 1,
	                      GTK_FILL, GTK_FILL, 0, 0);

      signal_detail = g_strconcat ("notify::", property_name, NULL);
      g_signal_connect_object (config, signal_detail, 
	                             G_CALLBACK (config_notify_label), button_label, 0);
	    g_free (signal_detail);

      closure = generate_create_scale_entry_closure (config, property_name,
                                                     text, step_increment, page_increment,
                                                     digits, limit_scale, lower_limit, upper_limit,
                                                     logarithm, opacity, TRUE);

      g_signal_connect_closure (button, "create-view", closure, 0);
    }
  else
    {
      if (opacity)
        {
          GtkObject *adj;
          adj   = gimp_prop_opacity_entry_new (config, property_name,
                                               GTK_TABLE (table), column, row,
                                               text);
        }
      else
        {
          GtkObject *adj;
          adj = gimp_prop_scale_entry_new (config, property_name,
                                           GTK_TABLE (table), 
                                           column, row,
                                           text,
                                           step_increment, 
                                           page_increment,
                                           digits,
                                           limit_scale, 
                                           lower_limit, 
                                           upper_limit);
          if (logarithm)
            gimp_scale_entry_set_logarithmic (GTK_OBJECT (adj), TRUE);    
        }
    }
}

void
gimp_tool_options_scale_entry_new (GObject      *config,
                                   const gchar *property_name,
                                   GtkTable     *table,
                                   gint          column,
                                   gint          row,
                                   const gchar *text,
                                   gdouble       step_increment,
                                   gdouble       page_increment,
                                   gint          digits,
                                   gboolean      limit_scale,
                                   gdouble       lower_limit,
                                   gdouble       upper_limit,
                                   gboolean      logarithm,
                                   gboolean      horizontal)
{
  gimp_tool_options_scale_entry_new_internal (config,
                                              property_name,
                                              table, column, row,
                                              text,
                                              step_increment, page_increment,
                                              digits, 
                                              limit_scale, lower_limit, upper_limit,
                                              logarithm, FALSE, horizontal);                                              
}

void
gimp_tool_options_opacity_entry_new (GObject     *config,
                   const gchar *property_name,
                   GtkTable    *table,
                   gint         column,
                   gint         row,
                   const gchar       *text,
                   gboolean     horizontal)
{
  gimp_tool_options_scale_entry_new_internal (config,
                                              property_name,
                                              table, column, row,
                                              text,
                                              0, 1,
                                              1, 
                                              TRUE, 0, 100,
                                              FALSE, TRUE, horizontal);
}

