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

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/temp-buf.h"

#include "core/gimpbrush.h"
#include "core/gimptoolinfo.h"

#include "paint/gimppaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpviewablebox.h"
#include "widgets/gimpwidgets-constructors.h"
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
#include "gimpbrushoptions-gui.h"
#include "gimpdynamicsoptions-gui.h"

#include "gimp-intl.h"

typedef void (*GimpContextNotifyCallback)   (GObject *config, GParamSpec *param_spec, GtkWidget *label);

#if 0
static GtkWidget * fade_options_gui      (GimpPaintOptions *paint_options,
                                           GType             tool_type,
                                           gboolean          horizontal);

static GtkWidget * gradient_options_gui  (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          GtkWidget        *incremental_toggle,
                                          gboolean          horizontal);
#endif
static GtkWidget * jitter_options_gui    (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          gboolean          horizontal);
static GtkWidget * smoothing_options_gui (GimpPaintOptions *paint_options,
                                          GType             tool_type,
                                          gboolean          horizontal);

static GtkWidget * dynamics_options_gui       (GimpPaintOptions *paint_options,
                                               GType             tool_type,
                                               gboolean          horizontal);

static void gimp_paint_options_gui_reset_size (GtkWidget        *button,
                                               GimpPaintOptions *paint_options);

static void dynamics_options_create_view     (GtkWidget *button, 
                                                GtkWidget **result, GObject *config);
static void       jitter_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
#if 0
static void       fade_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
static void       gradient_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);
#endif
static void       smoothing_options_create_view (GtkWidget *button, 
                                             GtkWidget **result, GObject *config);

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
  GtkWidget        *hbox;
  GtkWidget        *frame;
  GtkWidget        *table;
  GtkWidget        *menu;
  GtkWidget        *scale;
  GtkWidget        *label;
  GtkWidget        *button;
  GtkWidget        *incremental_toggle = NULL;
  GType             tool_type;
  GList            *children;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (horizontal);  

  tool_type = tool_options->tool_info->tool_type;

  /*  the main table  */
  table = gimp_tool_options_table (3, horizontal);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

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
  scale = gimp_prop_opacity_spin_scale_new (config, "opacity",
                                            _("Opacity"));
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the brush  */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL))
    {
      GtkWidget *button;

      if (horizontal)
        button = gimp_brush_button_with_popup (config);
      else
        button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT (tool_options), _("Brush:"), 2,
                                          "brush-view-type", "brush-view-size");
      gimp_table_attach_aligned (GTK_TABLE (table),
                                 gimp_tool_options_table_increment_get_col (&inc),
                                 gimp_tool_options_table_increment_get_row (&inc),
                                 _("Brush:"), 0.0, 0.5,
                                 button, 2, FALSE);
      gimp_tool_options_table_increment_next (&inc);

      /* brush size */
      if (horizontal)
        hbox = vbox;
      else
        {
          hbox = gtk_hbox_new (FALSE, 2);
          gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
          gtk_widget_show (hbox);
        }

      scale = gimp_prop_spin_scale_new (config, "brush-size",
                                        _("Size"),
                                        0.01, 1.0, 2);
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect (button, "clicked",
                        G_CALLBACK (gimp_paint_options_gui_reset_size),
                        options);

      gimp_help_set_help_data (button,
                               _("Reset size to brush's native size"), NULL);

      if (!horizontal)
        {
          scale = gimp_prop_spin_scale_new (config, "brush-aspect-ratio",
                                            _("Aspect Ratio"),
                                            0.01, 0.1, 2);
          gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
          gtk_widget_show (scale);

          scale = gimp_prop_spin_scale_new (config, "brush-angle",
                                            _("Angle"),
                                            1.0, 5.0, 2);
          gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
          gtk_widget_show (scale);

          button = gimp_prop_dynamics_box_new (NULL, GIMP_CONTEXT (tool_options),
                                               _("Dynamics"), 2,
                                               "dynamics-view-type",
                                               "dynamics-view-size");
          gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
          gtk_widget_show (button);
        }
      else
        {
          button = gimp_dynamics_button_with_popup (config);
          gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
          gtk_widget_show (button);
        }
      frame = dynamics_options_gui (options, tool_type, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);

      frame = jitter_options_gui (options, tool_type, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }

  /* the "smoothing" toggle */
  if (g_type_is_a (tool_type, GIMP_TYPE_BRUSH_TOOL) ||
      tool_type == GIMP_TYPE_INK_TOOL ||
      tool_type == GIMP_TYPE_SMUDGE_TOOL ||
      tool_type == GIMP_TYPE_DODGE_BURN_TOOL)
    {
      frame = smoothing_options_gui (options, tool_type, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }
  

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
#if 0
  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      frame = gradient_options_gui (options, tool_type, incremental_toggle, horizontal);
      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
      gtk_widget_show (frame);
    }
#endif    
  if (tool_type == GIMP_TYPE_SMUDGE_TOOL)
    {
      button = gimp_prop_check_button_new (config, "use-color-blending", _("Color Blending"));
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);
    }

  if (horizontal)
    {
      children = gtk_container_get_children (GTK_CONTAINER (vbox));  
      gimp_tool_options_setup_popup_layout (children, FALSE);
    }

  return vbox;
}


/*  private functions  */
static GtkWidget *
dynamics_options_gui (GimpPaintOptions *paint_options,
                      GType             tool_type,
                      gboolean          horizontal)
{
  return gimp_tool_options_expander_gui_with_popup (G_OBJECT (paint_options), tool_type,
                                             "dynamics-expanded", _("Dynamics"), _("Dynamics Options"),
                                             horizontal, dynamics_options_create_view);
}

static void
dynamics_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *inner_frame;
  GtkWidget *table;
  GtkWidget *scale;
  GtkWidget *menu;
  GtkWidget *combo;
  GtkWidget *checkbox;
  GtkWidget *vbox;
  GtkWidget *inner_vbox;
  GtkWidget *hbox;
  GtkWidget *box;

  vbox = gtk_vbox_new (FALSE, 2);

  inner_frame = gimp_frame_new (_("Fade Options"));
  gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
  gtk_widget_show (inner_frame);

  inner_vbox = gtk_vbox_new (FALSE, 2);
  gtk_container_add (GTK_CONTAINER (inner_frame), inner_vbox);
  gtk_widget_show (inner_vbox);

  /*  the fade-out scale & unitmenu  */
  hbox = gtk_hbox_new (FALSE, 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  scale = gimp_prop_spin_scale_new (config, "fade-length",
                                    _("Fade length"), 1.0, 50.0, 0);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  menu = gimp_prop_unit_combo_box_new (config, "fade-unit");
  gtk_box_pack_start (GTK_BOX (hbox), menu, FALSE, FALSE, 0);
  gtk_widget_show (menu);

  /*  the repeat type  */
  table = gtk_table_new (1, 2, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (inner_vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  combo = gimp_prop_enum_combo_box_new (config, "fade-repeat", 0, 0);
  gimp_table_attach_aligned (GTK_TABLE (table), 0, 0,
                             _("Repeat:"), 0.0, 0.5,
                             combo, 1, FALSE);

  checkbox = gimp_prop_check_button_new (config, "fade-reverse",
                                         _("Reverse"));
  gtk_box_pack_start (GTK_BOX (inner_vbox), checkbox, FALSE, FALSE, 0);
  gtk_widget_show (checkbox);
  

  /* Color UI */
//  if (g_type_is_a (tool_type, GIMP_TYPE_PAINTBRUSH_TOOL))
    {
      inner_frame = gimp_frame_new (_("Color Options"));
      gtk_box_pack_start (GTK_BOX (vbox), inner_frame, FALSE, FALSE, 0);
      gtk_widget_show (inner_frame);

      box = gimp_prop_gradient_box_new (NULL, GIMP_CONTEXT (config),
                                        _("Gradient"), 2,
                                        "gradient-view-type",
                                        "gradient-view-size",
                                        "gradient-reverse");
      gtk_container_add (GTK_CONTAINER (inner_frame), box);
      gtk_widget_show (box);
    }

//  if (horizontal)
    {
      GList *children;
      children = gtk_container_get_children (GTK_CONTAINER (hbox));  
      gimp_tool_options_setup_popup_layout (children, FALSE);
    }

  *result = vbox;
}

static GtkWidget *
jitter_options_gui (GimpPaintOptions           *paint_options,
                    GType                       tool_type,
                    gboolean                    horizontal)
{
  return gimp_tool_options_toggle_gui_with_popup (G_OBJECT (paint_options), tool_type,
                             "use-jitter", _("Jitter"), _("Apply Jitter"),
                             horizontal, jitter_options_create_view);
}

static void
jitter_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *vbox;
  GtkWidget *scale;
  GList     *children;

  vbox = gtk_vbox_new (FALSE, 2);
  scale = gimp_prop_spin_scale_new (config, "jitter-amount",
                                    _("Amount"),
                                    0.01, 1.0, 2);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  children = gtk_container_get_children (GTK_CONTAINER (vbox));
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = vbox;
}

static void
gimp_paint_options_gui_reset_size (GtkWidget        *button,
                                   GimpPaintOptions *paint_options)
{
 GimpBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

 if (brush)
   {
     g_object_set (paint_options,
                   "brush-size", (gdouble) MAX (brush->mask->width,
                                                brush->mask->height),
                   NULL);
   }
}

#if 0
static GtkWidget *
gradient_options_gui (GimpPaintOptions         *paint_options,
                    GType                       tool_type,
                    GtkWidget                  *incremental_toggle,
                    gboolean                    horizontal)
{
  return gimp_tool_options_toggle_gui_with_popup (G_OBJECT (paint_options), tool_type,
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
#endif

static GtkWidget *
smoothing_options_gui (GimpPaintOptions         *paint_options,
                    GType                       tool_type,
                    gboolean                    horizontal)
{
  return gimp_tool_options_toggle_gui_with_popup (G_OBJECT (paint_options), tool_type,
                             "use-smoothing", _("Smoothing"), _("Apply Smoothing"),
                             horizontal, smoothing_options_create_view);
}

static void
smoothing_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *vbox;
  GtkWidget *scale;
  GList     *children;

  vbox = gtk_vbox_new (FALSE, 2);
  scale = gimp_prop_spin_scale_new (config, "smoothing-history",
                                    _("Quality:"),
                                    1, 20, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "smoothing-factor",
                                    _("Factor:"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);
  
  children = gtk_container_get_children (GTK_CONTAINER (vbox));
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = vbox;
}

