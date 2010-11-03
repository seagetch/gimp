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

#include <gtk/gtk.h>
#include <glib/gprintf.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimppopupbutton.h"
#include "widgets/gimpwidgets-utils.h"

#include "gimpairbrushtool.h"
#include "gimpclonetool.h"
#include "gimpconvolvetool.h"
#include "gimpdodgeburntool.h"
#include "gimperasertool.h"
#include "gimphealtool.h"
#include "gimpinktool.h"
#include "gimppaintoptions-gui.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpsmudgetool.h"
#include "gimptooloptions-gui.h"

#include "gimp-intl.h"

typedef void (*GimpPopupCreateViewCallback) (GtkWidget *button, GtkWidget** result, GObject *config);
typedef void (*GimpContextNotifyCallback)   (GObject *config, GParamSpec *param_spec, GtkWidget *label);

static GtkWidget * fade_options_gui      (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          gboolean          horizontal);
static GtkWidget * gradient_options_gui  (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkWidget        *incremental_toggle,
                                          gboolean          horizontal);
static GtkWidget * jitter_options_gui    (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          gboolean          horizontal);
static GtkWidget * smoothing_options_gui (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          gboolean          horizontal);

static void       fade_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       jitter_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       gradient_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       smoothing_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);

static void       aspect_entry_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       scale_entry_create_view  (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       angle_entry_create_view  (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       opacity_entry_create_view  (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);

static GtkWidget * popup_options_gui       (GimpPaintOptions           *paint_options,
                                             GType                       tool_type, 
                                             gchar                      *property_name,
                                             gchar                      *short_label,
                                             gchar                      *long_label,
                                             gboolean                    horizontal,
                                             GimpPopupCreateViewCallback create_view);
static void       scale_entry_gui          (GObject     *config,
                                             const gchar *property_name,
                                             GtkTable    *table,
                                             gint         column,
                                             gint         row,
                                             const gchar *label,
                                             gdouble      step_increment,
                                             gdouble      page_increment,
                                             gint         digits,
                                             gboolean     limit_scale,
                                             gdouble      lower_limit,
                                             gdouble      upper_limit,
                                             gboolean     logarithmic,
                                             gboolean     percentage,
                                             gboolean     horizontal,
                                             GimpPopupCreateViewCallback create_view);
static void opacity_entry_gui             (GObject     *config,
                                            const gchar *property_name,
                                            GtkTable    *table,
                                            gint         column,
                                            gint         row,
                                            const gchar *text,
                                            gboolean     horizontal,
                                            GimpPopupCreateViewCallback create_view);
/*  public functions  */

GtkWidget *
gimp_paint_options_gui (GimpToolOptions *tool_options)
{
  return gimp_paint_options_gui_full (tool_options, FALSE);
}

GtkWidget *
gimp_paint_options_gui_horizontal (GimpToolOptions *tool_options)
{
  return gimp_paint_options_gui_full (tool_options, TRUE);
}

GtkWidget *
gimp_paint_options_gui_full (GimpToolOptions *tool_options, gboolean horizontal)
{
  GObject          *config  = G_OBJECT (tool_options);
  GimpPaintOptions *options = GIMP_PAINT_OPTIONS (tool_options);
  GtkWidget        *vbox    = gimp_tool_options_gui_full (tool_options, horizontal);
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *label;
  GtkWidget        *button;
  GtkWidget        *incremental_toggle = NULL;
  GType             tool_type;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (horizontal);  

  tool_type = tool_options->tool_info->tool_type;

  /*  the main table  */
  table = gimp_tool_options_table (3, horizontal);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  g_object_set_data (G_OBJECT (vbox), GIMP_PAINT_OPTIONS_TABLE_KEY, table);

  /*  the paint mode menu  */
  menu  = gimp_prop_paint_mode_menu_new (config, "paint-mode", TRUE, FALSE);
  label = gimp_table_attach_aligned (GTK_TABLE (table), 
                                     gimp_tool_options_table_increment_get_col (&inc),
                                     gimp_tool_options_table_increment_get_row (&inc),
                                     _("Mode:"), 0.0, 0.5,
                                     menu, 2, FALSE);
  gimp_tool_options_table_increment_next (&inc);

  if (tool_type == GIMP_TYPE_ERASER_TOOL     ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL   ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      gtk_widget_set_sensitive (menu, FALSE);
      gtk_widget_set_sensitive (label, FALSE);
    }

  /*  the opacity scale  */
  opacity_entry_gui (config, "opacity",
                               GTK_TABLE (table),
                               gimp_tool_options_table_increment_get_col (&inc),
                               gimp_tool_options_table_increment_get_row (&inc),
                               _("Opacity:"),
                               horizontal, opacity_entry_create_view);
  gimp_tool_options_table_increment_next (&inc);

  /*  the brush  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *button;


      button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT (tool_options), 2,
                                        "brush-view-type", "brush-view-size");
      gimp_table_attach_aligned (GTK_TABLE (table),
                                 gimp_tool_options_table_increment_get_col (&inc),
                                 gimp_tool_options_table_increment_get_row (&inc),
                                 _("Brush:"), 0.0, 0.5,
                                 button, 2, FALSE);
      gimp_tool_options_table_increment_next (&inc);

      scale_entry_gui (config, "brush-scale",
                       GTK_TABLE (table), 
                       gimp_tool_options_table_increment_get_col (&inc),
                       gimp_tool_options_table_increment_get_row (&inc),
                       _("Scale:"),
                       0.01, 0.1, 2,
                       FALSE, 0.0, 0.0,
                       TRUE, FALSE, horizontal, scale_entry_create_view);
      gimp_tool_options_table_increment_next (&inc);

      scale_entry_gui (config, "brush-aspect-ratio",
                       GTK_TABLE (table),
                       gimp_tool_options_table_increment_get_col (&inc),
                       gimp_tool_options_table_increment_get_row (&inc),
                       /* Label for a slider that affects
                          aspect ratio for brushes */
                       _("Aspect:"),
                       0.01, 0.1, 2,
                       FALSE, 0.0, 0.0,
                       TRUE, FALSE, horizontal, aspect_entry_create_view);
      gimp_tool_options_table_increment_next (&inc);

      scale_entry_gui (config, "brush-angle",
                       GTK_TABLE (table),
                       gimp_tool_options_table_increment_get_col (&inc),
                       gimp_tool_options_table_increment_get_row (&inc),
                       _("Angle:"),
                       1.0, 5.0, 2,
                       FALSE, 0.0, 0.0,
                       FALSE, FALSE, horizontal, angle_entry_create_view);
      gimp_tool_options_table_increment_next (&inc);
    }


  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      frame = fade_options_gui (options, tool_type, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame = jitter_options_gui (options, tool_type, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

    frame = smoothing_options_gui (options, tool_type, horizontal);
    gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
    gtk_widget_show (frame);
    

  /*  the "incremental" toggle  */
  if (tool_type == GIMP_TYPE_PENCIL_TOOL     ||
      tool_type == GIMP_TYPE_PAINTBRUSH_TOOL ||
      tool_type == GIMP_TYPE_ERASER_TOOL)
    {
      incremental_toggle =
        gimp_prop_enum_check_button_new (config,
                                         "application-mode",
                                         _("Incremental"),
                                         GIMP_PAINT_CONSTANT,
                                         GIMP_PAINT_INCREMENTAL);
      gtk_box_pack_start (GTK_BOX (vbox), incremental_toggle, FALSE, FALSE, 0);
      gtk_widget_show (incremental_toggle);
    }

  /* the "hard edge" toggle */
  if (tool_type == GIMP_TYPE_ERASER_TOOL            ||
      tool_type == GIMP_TYPE_CLONE_TOOL             ||
      tool_type == GIMP_TYPE_HEAL_TOOL              ||
      tool_type == GIMP_TYPE_PERSPECTIVE_CLONE_TOOL ||
      tool_type == GIMP_TYPE_CONVOLVE_TOOL          ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL        ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "hard", _("Hard edge"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      frame = gradient_options_gui (options, tool_type, incremental_toggle, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }
    
  if (tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "use-color-blending", _("Color Blending"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  return vbox;
}


/*  private functions  */

static GtkWidget *
popup_options_gui (GimpPaintOptions           *paint_options,
                   GType                       tool_type, 
                   gchar                      *property_name,
                   gchar                      *short_label,
                   gchar                      *long_label,
                   gboolean                    horizontal,
                   GimpPopupCreateViewCallback create_view)
{
  GtkWidget *result;
  GObject   *config = G_OBJECT (paint_options);

  if (horizontal)
    {
      GtkWidget  *button_label;
      GtkWidget  *button;
      GtkBox     *hbox;
      
      hbox          = GTK_BOX (gtk_hbox_new (FALSE, 0));
      button_label  = gtk_label_new ("+");
      
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

static GtkWidget *
fade_options_gui (GimpPaintOptions            *paint_options,
                  GType                        tool_type, 
                  gboolean                     horizontal)
{
  return popup_options_gui (paint_options, tool_type, 
                             "use-fade", _("Fade"), _("Fade out"),
                             horizontal, fade_options_create_view);
}

static void
fade_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkWidget *spinbutton;
  GtkWidget *menu;
  GtkWidget *combo;
  GtkWidget *checkbox;
  GList     *children;

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);

  /*  the fade-out sizeentry  */
  spinbutton = gimp_prop_spin_button_new (config, "fade-length",
                                          1.0, 50.0, 0);
  gtk_entry_set_width_chars (GTK_ENTRY (spinbutton), 6);

  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Length:"), 0.0, 0.5,
                             spinbutton, 1, FALSE);

  /*  the fade-out unitmenu  */
  menu = gimp_prop_unit_combo_box_new (config, "fade-unit");
  gtk_table_attach (GTK_TABLE (table), menu, 2, 3, 0, 1,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (menu);

#if 0
  /* FIXME pixel digits */
  g_object_set_data (G_OBJECT (menu), "set_digits", spinbutton);
  gimp_unit_menu_set_pixel_digits (GIMP_UNIT_MENU (menu), 0);
#endif

    /*  the repeat type  */
  combo = gimp_prop_enum_combo_box_new (config, "fade-repeat", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 2,
                             _("Repeat:"), 0.0, 0.5,
                             combo, 2, FALSE);

  checkbox = gimp_prop_check_button_new (config,
                                         "fade-reverse",
                                          _("Reverse"));
  gtk_table_attach (GTK_TABLE (table), checkbox, 0, 2, 3, 4,
                    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_widget_show (checkbox);
  

  children = gtk_container_get_children (GTK_CONTAINER (table));  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_HSCALE (widget))
        gtk_widget_set_size_request (widget, 100, -1);
        
      children = next;
    }

  *result = table;
}

static GtkWidget *
jitter_options_gui (GimpPaintOptions           *paint_options,
                    GType                       tool_type,
                    gboolean                    horizontal)
{
  return popup_options_gui (paint_options, tool_type,
                             "use-jitter", _("Jitter"), _("Apply Jitter"),
                             horizontal, jitter_options_create_view);
}

static void
jitter_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GList     *children;

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);

  gimp_prop_scale_entry_new (config, "jitter-amount",
                             GTK_TABLE (table), 0, 0,
                             _("Amount:"),
                             0.01, 0.1, 2,
                             TRUE, 0.0, 5.0);

  children = gtk_container_get_children (GTK_CONTAINER (table));
  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }

  *result = table;
}

static GtkWidget *
gradient_options_gui (GimpPaintOptions         *paint_options,
                    GType                       tool_type,
                    GtkWidget                  *incremental_toggle,
                    gboolean                    horizontal)
{
  return popup_options_gui (paint_options, tool_type,
                             "use-gradient", _("Gradient"), _("Use color from gradient"),
                             horizontal, gradient_options_create_view);
}

static void
gradient_options_create_view (GtkWidget *event_target, GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkWidget *button;

  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_table_set_row_spacings (GTK_TABLE (table), 2);

/*
  if (incremental_toggle)
    {
      gtk_widget_set_sensitive (incremental_toggle,
                                ! paint_options->gradient_options->use_gradient);
      g_object_set_data (G_OBJECT (button), "inverse_sensitive",
                         incremental_toggle);
    }
*/

  /*  the gradient view  */
  button = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (config), 2,
                                       "gradient-view-type",
                                       "gradient-view-size",
                                       "gradient-reverse");
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Gradient:"), 0.0, 0.5,
                             button, 2, TRUE);

  *result = table;
}

static GtkWidget *
smoothing_options_gui (GimpPaintOptions         *paint_options,
                    GType                       tool_type,
                    gboolean                    horizontal)
{
  return popup_options_gui (paint_options, tool_type,
                             "use-smoothing", _("Smoothing"), _("Apply Smoothing"),
                             horizontal, smoothing_options_create_view);
}

static void
smoothing_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkObject *factor;
  GList     *children;

  table = gtk_table_new (2, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);

  gimp_prop_scale_entry_new (config, "smoothing-history",
                             GTK_TABLE (table), 0, 0,
                             _("Quality:"),
                             1, 10, 1,
                             FALSE, 0, 100);
                             
  factor = gimp_prop_scale_entry_new (config, "smoothing-factor",
                                      GTK_TABLE (table), 0, 1,
                                      _("Factor:"),
                                      1, 10, 1,
                                      FALSE, 0, 100);
  gimp_scale_entry_set_logarithmic (factor, TRUE);
  
  children = gtk_container_get_children (GTK_CONTAINER (table));
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }

  *result = table;
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
scale_entry_gui (GObject     *config,
                 const gchar *property_name,
                 GtkTable    *table,
                 gint         column,
                 gint         row,
                 const gchar *text,
                 gdouble      step_increment,
                 gdouble      page_increment,
                 gint         digits,
                 gboolean     limit_scale,
                 gdouble      lower_limit,
                 gdouble      upper_limit,
                 gboolean     logarithmic,
                 gboolean     percentage,
                 gboolean     horizontal,
                 GimpPopupCreateViewCallback create_view)
{
  if (horizontal)
    {
      gchar      *signal_detail;
      GtkWidget  *button_label;
      GtkWidget  *label;
      GtkWidget  *button;
      GParamSpec *param_spec;
      
      button_label  = gtk_label_new ("");
      button        = gimp_popup_button_new (button_label);
      param_spec    = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                    property_name);
      label         = gtk_label_new (text);

      if (percentage)
        g_object_set_data (G_OBJECT (button_label), "percentage", GINT_TO_POINTER (FALSE));

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

      g_signal_connect_object (button, "create-view", G_CALLBACK (create_view), config, 0);
    }
  else
    {
      GtkObject *adj;
    
      adj = gimp_prop_scale_entry_new (config, property_name,
                                       GTK_TABLE (table), 
                                       column, row,
                                       text,
                                       step_increment, page_increment, digits,
                                       limit_scale, lower_limit, upper_limit);
      gimp_scale_entry_set_logarithmic (adj, logarithmic);
    }
}

static void
opacity_entry_gui (GObject     *config,
                   const gchar *property_name,
                   GtkTable    *table,
                   gint         column,
                   gint         row,
                   const gchar *text,
                   gboolean     horizontal,
                   GimpPopupCreateViewCallback create_view)
{
  if (horizontal)
    {
      gchar      *signal_detail;
      GtkWidget  *button_label;
      GtkWidget  *label;
      GtkWidget  *button;
      GParamSpec *param_spec;
      
      button_label  = gtk_label_new ("");
      button        = gimp_popup_button_new (button_label);
      param_spec    = g_object_class_find_property (G_OBJECT_GET_CLASS (config),
                                                    property_name);
      label         = gtk_label_new (text);

      g_object_set_data (G_OBJECT (button_label), "percentage", GINT_TO_POINTER (TRUE));
      g_object_set_data (G_OBJECT (button_label), "digits", GINT_TO_POINTER (1));
      
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

      g_signal_connect_object (button, "create-view", G_CALLBACK (create_view), config, 0);
    }
  else
    {
      GtkObject *adj;
    
      adj = gimp_prop_opacity_entry_new (config,
                                         property_name,
                                         GTK_TABLE (table),
                                         column,
                                         row,
                                         text);
    }
}

static void       
opacity_entry_create_view (GtkWidget *button, 
                         GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkObject *adj;
  GList     *children;

  table = gtk_table_new (1, 3, FALSE);
  adj   = gimp_prop_opacity_entry_new (config, "opacity",
                                       GTK_TABLE (table), 0, 0,
                                       "");

  children = gtk_container_get_children (GTK_CONTAINER (table));
  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }

  *result = table;
}

static void       
aspect_entry_create_view (GtkWidget *button, 
                         GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkObject *adj;
  GList     *children;

  table = gtk_table_new (1, 3, FALSE);
  adj   = gimp_prop_scale_entry_new (config, "brush-aspect-ratio",
                                     GTK_TABLE (table),
                                     0, 0,
                                     "",
                                     0.01, 0.1, 2,
                                     FALSE, 0.0, 0.0);

  children = gtk_container_get_children (GTK_CONTAINER (table));
  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }
  *result = table;
}

static void       
scale_entry_create_view (GtkWidget *button, 
                         GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkObject *adj;
  GList     *children;

  table = gtk_table_new (1, 3, FALSE);
  adj = gimp_prop_scale_entry_new (config, "brush-scale",
                                   GTK_TABLE (table), 
                                   0, 0,
                                   "",
                                   0.01, 0.1, 2,
                                   FALSE, 0.0, 0.0);

  children = gtk_container_get_children (GTK_CONTAINER (table));
  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }

  gimp_scale_entry_set_logarithmic (GTK_OBJECT (adj), TRUE);
  *result = table;
}


static void       
angle_entry_create_view (GtkWidget *button, 
                         GtkWidget **result, GObject *config)
{
  GtkWidget *table;
  GtkObject *adj;
  GList     *children;

  table = gtk_table_new (1, 3, FALSE);
  adj   = gimp_prop_scale_entry_new (config, "brush-angle",
                                     GTK_TABLE (table), 0, 0,
                                     "",
                                     1.0, 5.0, 2,
                                     FALSE, 0.0, 0.0);

  children = gtk_container_get_children (GTK_CONTAINER (table));
  
  while (children)
    {
      GtkWidget *widget = GTK_WIDGET (children->data);
      GList *next = g_list_next (children);
      if (GTK_IS_LABEL (widget))
        {
          gtk_widget_destroy (widget);
        }
      else if (GTK_IS_HSCALE (widget))
        {
          gtk_widget_set_size_request (widget, 100, -1);
        }
        
      children = next;
    }

  *result = table;
}

