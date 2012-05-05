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

#include <glib.h>
#include <glib/gprintf.h>
#include <gtk/gtk.h>

#include "libgimpwidgets/gimpwidgets.h"

#include "tools-types.h"

#include "base/temp-buf.h"

#include "core/gimpmypaintbrush.h"
#include "core/gimptoolinfo.h"
#include "core/mypaintbrush-brushsettings.h"

#include "paint/gimpmypaintoptions.h"

#include "widgets/gimppropwidgets.h"
#include "widgets/gimpspinscale.h"
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
#include "gimpmypaintoptions-gui.h"
#include "gimppenciltool.h"
#include "gimpperspectiveclonetool.h"
#include "gimpsmudgetool.h"
#include "gimptooloptions-gui.h"
#include "gimpmypaintbrushoptions-gui.h"
};

#include "gimptooloptions-gui-cxx.hpp"
#include "base/scopeguard.hpp"

#include "gimp-intl.h"

///////////////////////////////////////////////////////////////////////////////
class MypaintGUIPrivateBase {
protected:
  GimpMypaintOptions* options;

  class MypaintOptionsPropertyGUIPrivate {
    typedef MypaintOptionsPropertyGUIPrivate Class;
    GimpMypaintOptions* options;
    MyPaintBrushSettings* setting;
    
    GtkWidget* widget;
    gchar*   property_name;
    gchar*   internal_name;
    GClosure* notify_closure;
    GClosure* value_changed_closure;

  public:
    MypaintOptionsPropertyGUIPrivate(GimpMypaintOptions* opts,
				     GHashTable* dict,
				     const gchar* name) 
    {
      ScopeGuard<gchar, void(gpointer)> name_replaced(g_strdup(name), g_free);
      g_strcanon(name_replaced.ptr(), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_",'_');
      setting = reinterpret_cast<MyPaintBrushSettings*>(g_hash_table_lookup(dict, name_replaced.ptr()));
      internal_name = (gchar*)g_strdup(name);
      property_name = (gchar*)g_strdup_printf("notify::%s", name);
      options = opts;
      g_object_add_weak_pointer(G_OBJECT(options), (void**)&options);
      notify_closure = value_changed_closure = NULL;
    }
      
    ~MypaintOptionsPropertyGUIPrivate() {
      if (property_name)
        g_free(property_name);

      if (internal_name)
	g_free(internal_name);
      
/*      if (widget && value_changed_closure) {
	  gulong handler_id = g_signal_handler_find(gpointer(widget), G_SIGNAL_MATCH_CLOSURE, 0, 0, value_changed_closure, NULL, NULL);
	  g_signal_handler_disconnect(gpointer(widget), handler_id);
	  value_changed_closure = NULL;
      }

      if (options && notify_closure) {
        gulong handler_id = g_signal_handler_find(gpointer(optios), G_SIGNAL_MATCH_CLOSURE, 0, 0, notify_closure, NULL, NULL);
	g_signal_handler_disconnect(gpointer(options), handler_id);
	value_changed_closure = NULL;
      }*/
    }
      
    void notify(GObject* object) {
      gdouble value;
      g_object_get(G_OBJECT(options), internal_name, &value, NULL);
    }

    void value_changed(GObject* object) {
      gdouble value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(object));
      g_object_set(G_OBJECT(options), property_name, value, NULL);
    }
      
    GtkWidget* create() {
      gdouble range = setting->maximum - setting->minimum;
      widget  = gimp_prop_spin_scale_new (G_OBJECT(options), 
                                          internal_name,
					  _(setting->displayed_name),
                                          range / 1000.0, range / 100.0, 2);
      g_object_add_weak_pointer(G_OBJECT(widget), (void**)&widget);
      gimp_spin_scale_set_scale_limits (GIMP_SPIN_SCALE (widget), 
					setting->minimum, 
					setting->maximum);

/*      value_changed_closure = 
        g_signal_connect_delegator(G_OBJECT(widget),
				   "value-changed",
				   Delegator::delegator(this, &Class::value_changed));
      notify_closure = 
        g_signal_connect_delegator(G_OBJECT(options),
				   property_name,
				   Delegator::delegator(this, &Class::notify));*/
      g_object_set_cxx_object (G_OBJECT(widget), "behavior", this);
//      gtk_widget_set_size_request (widget, 200, -1);
      return widget;
    }
  };

public:
  MypaintGUIPrivateBase(GimpToolOptions* opts);
  virtual ~MypaintGUIPrivateBase() {}
  virtual GtkWidget* create() = 0;
};

MypaintGUIPrivateBase::
MypaintGUIPrivateBase(GimpToolOptions* opts)
{
  options = GIMP_MYPAINT_OPTIONS(opts);
}

///////////////////////////////////////////////////////////////////////////////
class MypaintOptionsGUIPrivate : public MypaintGUIPrivateBase {
  bool is_toolbar;
  
public:
  MypaintOptionsGUIPrivate(GimpToolOptions* options, bool toolbar);
  GtkWidget* create();

  void destroy(GObject* o);
  void create_basic_options(GObject* object, GtkWidget** result);
  void reset_size(GObject *o);
};


MypaintOptionsGUIPrivate::
MypaintOptionsGUIPrivate(GimpToolOptions* opts, bool toolbar) :
  MypaintGUIPrivateBase(opts), is_toolbar(toolbar)
{
}

GtkWidget *
MypaintOptionsGUIPrivate::create ()
{
  GObject            *config  = G_OBJECT (options);
  GtkWidget          *vbox    = gimp_tool_options_gui_full (GIMP_TOOL_OPTIONS(options), is_toolbar);
  GtkWidget          *hbox;
  GtkWidget          *frame;
  GtkWidget          *table;
  GtkWidget          *menu;
  GtkWidget          *scale;
  GtkWidget          *label;
  GtkWidget          *button;
  GtkWidget          *incremental_toggle = NULL;
  GType             tool_type;
  GList            *children;
  GimpToolOptionsTableIncrement inc = gimp_tool_options_table_increment (is_toolbar);  

  tool_type = GIMP_TOOL_OPTIONS(options)->tool_info->tool_type;
  ScopeGuard<GHashTable, void(GHashTable*)> brush_settings_dict(mypaint_brush_get_brush_settings_dict(), g_hash_table_unref);

  /*  the main table  */
  table = gimp_tool_options_table (3, is_toolbar);
  gtk_table_set_col_spacings (GTK_TABLE (table), 2);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  /*  the opacity scale  */
  scale = gimp_prop_opacity_spin_scale_new (config, "opacity",
                                            _("Opacity"));
//  gtk_widget_set_size_request (scale, 200, -1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, FALSE, FALSE, 0);
  gtk_widget_show (scale);

  /*  the brush  */
    {
      GtkWidget *button;
      MypaintOptionsPropertyGUIPrivate* scale_obj;
      
      if (is_toolbar)
        button = gimp_mypaint_brush_button_with_popup (config);
      else {
        button = gimp_mypaint_brush_button_with_popup (config);
/*        button = gimp_prop_brush_box_new (NULL, GIMP_CONTEXT(options),
                                          _("MypaintBrush"), 2,
                                          "mypaint-brush-view-type", "mypaint-brush-view-size",
                                          "gimp-mypaint-brush-editor");*/
      }
      gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      /* brush size */
      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }

      scale_obj = 
	new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "radius-logarithmic");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);
      gtk_widget_show (button);

      g_signal_connect_delegator (G_OBJECT(button), "clicked",
                                  Delegator::delegator(this, &MypaintOptionsGUIPrivate::reset_size));

      gimp_help_set_help_data (button,
                               _("Reset size to brush's native size"), NULL);

      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }

      scale_obj = 
	new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "slow-tracking");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

      if (is_toolbar)
        hbox = vbox;
      else {
        hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
        gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
        gtk_widget_show (hbox);
      }
      scale_obj = 
	new MypaintOptionsPropertyGUIPrivate(options, brush_settings_dict.ptr(), "hardness");
      scale = scale_obj->create();
      gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
      gtk_widget_show (scale);

//      frame = dynamics_options_gui (options, tool_type, is_toolbar);
//      gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
//      gtk_widget_show (frame);
    }

  if (is_toolbar)
    {
      children = gtk_container_get_children (GTK_CONTAINER (vbox));  
      gimp_tool_options_setup_popup_layout (children, FALSE);
    }

  g_object_set_cxx_object(G_OBJECT(vbox), "behavior-options-private", this);
  gtk_widget_show(vbox);
  return vbox;
}

void
MypaintOptionsGUIPrivate::reset_size (GObject* o)
{
/*
 GimpMypaintBrush *brush = gimp_context_get_brush (GIMP_CONTEXT (paint_options));

 if (brush)
   {
     g_object_set (paint_options,
                   "brush-size", (gdouble) MAX (brush->mask->width,
                                                brush->mask->height),
                   NULL);
   }
*/
}

///////////////////////////////////////////////////////////////////////////////
class MypaintBrushEditorPrivate : public MypaintGUIPrivateBase {
public:
  MypaintBrushEditorPrivate(GimpToolOptions* opts) :
    MypaintGUIPrivateBase(opts)
  {
  }
  GtkWidget* create();
};

GtkWidget*
MypaintBrushEditorPrivate::create() {
/*
  GimpDataEditor *data_editor = GIMP_DATA_EDITOR (object);
  GimpMypaintBrushEditor* editor = GIMP_MYPAINT_BRUSH_EDITOR (object);

  if (G_OBJECT_CLASS (parent_class)->constructed)
    G_OBJECT_CLASS (parent_class)->constructed (object);

  GtkWidget      *frame;
  GtkWidget      *hbox;
  GtkWidget      *label;
  GtkWidget      *box;

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (editor), frame, TRUE, TRUE, 0);
  gtk_widget_show (frame);

  data_editor->view = gimp_view_new_full_by_types (NULL,
                                                   GIMP_TYPE_VIEW,
                                                   GIMP_TYPE_MYPAINT_BRUSH,
                                                   MYPAINT_BRUSH_VIEW_SIZE,
                                                   MYPAINT_BRUSH_VIEW_SIZE, 0,
                                                   FALSE, FALSE, TRUE);
  gtk_widget_set_size_request (data_editor->view, -1, MYPAINT_BRUSH_VIEW_SIZE);
  gimp_view_set_expand (GIMP_VIEW (data_editor->view), TRUE);
  gtk_container_add (GTK_CONTAINER (frame), data_editor->view);
  gtk_widget_show (data_editor->view);
*/
  GtkWidget* brushsetting_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show(brushsetting_vbox);


  GList* setting_groups;
  setting_groups = mypaint_brush_get_setting_group_list();
/*
  GimpContext* context;
  g_object_get(object, "context", &context, NULL);
 */ 
    

  for (GList* iter = setting_groups; iter; iter = iter->next) {
    MypaintBrushSettingGroup* group =
      (MypaintBrushSettingGroup*)iter->data;
//    self.visible_settings = self.visible_settings + group['settings']

    ScopeGuard<gchar, void(gpointer)> bold_title(
      g_strdup_printf("<b>%s</b>", group->display_name), g_free);    
    GtkWidget* group_expander = gtk_expander_new(bold_title.ptr());
    gtk_expander_set_use_markup(GTK_EXPANDER(group_expander), TRUE);
    gtk_widget_show(group_expander);

    GtkWidget* table = gtk_table_new(3, g_list_length(group->setting_list), FALSE);
    gtk_widget_show(table);

    if (strcmp(group->name, "basic") == 0)
	gtk_expander_set_expanded (GTK_EXPANDER(group_expander), TRUE);

    int count = 0;
    for (GList* iter2 = group->setting_list; iter2; iter2 = iter2->next, count ++) {
      MyPaintBrushSettings* s = (MyPaintBrushSettings*)iter2->data;
      ScopeGuard<gchar,void(gpointer)> prop_name(g_strdup(s->internal_name), g_free);
      g_strcanon(prop_name.ptr(), "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-",'-');
      GtkWidget* h = gimp_prop_spin_scale_new(G_OBJECT(options), prop_name.ptr(),
                               s->displayed_name,
                               0.1, 1.0, 2);
      gtk_widget_set_tooltip_text(h, s->tooltip);
      gtk_widget_show(h);

      GtkWidget* button = gimp_stock_button_new (GIMP_STOCK_RESET, NULL);
      gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
      gtk_image_set_from_stock (GTK_IMAGE (gtk_bin_get_child (GTK_BIN (button))),
                                GIMP_STOCK_RESET, GTK_ICON_SIZE_MENU);
      gtk_widget_show (button);
      gtk_widget_set_tooltip_text(button, _("Reset to default value"));
      
      //g_signal_connect_...(button, "clicked,...);
      GtkWidget* button2;
      if (s->constant) {
        button2 = gtk_label_new("");
	gtk_widget_set_tooltip_text(button2, _("No additional configuration"));
	gtk_misc_set_alignment(GTK_MISC(button2), 0.5, 0.5);
      } else {
        button2 = gtk_button_new_with_label("...");
	gtk_widget_set_tooltip_text(button2, _("Add input value mapping"));
        //g_signal_connect...(button2, "clicked", self.details_clicked_cb, adj, s)
      }
      gtk_widget_show(button2);
      gtk_table_attach(GTK_TABLE(table), h, 0, 1, count, count+1, 
                       GtkAttachOptions(GTK_EXPAND | GTK_FILL), 
                       GtkAttachOptions(GTK_EXPAND | GTK_FILL), 0, 0);
      gtk_table_attach(GTK_TABLE(table), button, 1, 2, count, count+1, 
                       GtkAttachOptions(GTK_FILL), 
                       GtkAttachOptions(GTK_FILL), 0, 0);
      gtk_table_attach(GTK_TABLE(table), button2, 2, 3, count, count+1, 
                       GtkAttachOptions(GTK_FILL), 
                       GtkAttachOptions(GTK_FILL), 0, 0);
    }
    gtk_container_add(GTK_CONTAINER(group_expander), table);
    gtk_box_pack_start(GTK_BOX(brushsetting_vbox) ,group_expander,TRUE,TRUE,0);
  }
  
  g_object_set_cxx_object(G_OBJECT(brushsetting_vbox), "behavior-editor-private", this);
  return brushsetting_vbox;
}


///////////////////////////////////////////////////////////////////////////////
static void
smoothing_options_create_view (GtkWidget *button, GtkWidget **result, GObject *config)
{
  GtkWidget *vbox;
  GtkWidget *scale;
  GList     *children;

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
#if 0
  frame = gimp_prop_expanding_frame_new (config, "use-smoothing",
                                         _("Smooth stroke"),
                                         vbox, NULL);
#endif
  scale = gimp_prop_spin_scale_new (config, "smoothing-quality",
                                    _("Quality"),
                                    1, 20, 0);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);

  scale = gimp_prop_spin_scale_new (config, "smoothing-factor",
                                    _("Weight"),
                                    1, 10, 1);
  gtk_box_pack_start (GTK_BOX (vbox), scale, TRUE, TRUE, 0);
  gtk_widget_show (scale);
  
  children = gtk_container_get_children (GTK_CONTAINER (vbox));
  gimp_tool_options_setup_popup_layout (children, FALSE);

  *result = vbox;
}


///////////////////////////////////////////////////////////////////////////////
/*  public functions  */
extern "C" {

GtkWidget *
gimp_mypaint_options_gui (GimpToolOptions *tool_options)
{
  g_print("MYPAINTOPTIONSGUI::create...\n");
  MypaintOptionsGUIPrivate* priv = new MypaintOptionsGUIPrivate(tool_options, false);
  MypaintBrushEditorPrivate* editor_priv = new MypaintBrushEditorPrivate(tool_options);
  GtkWidget* priv_widget = priv->create();
  GtkWidget* editor_widget = editor_priv->create();

  GtkWidget* options_box = gtk_notebook_new();
  gtk_widget_show (options_box);
//  gtk_notebook_set_show_tabs(GTK_NOTEBOOK(options_box), FALSE);

//  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), brushsetting_vbox, label, 0);
//  gtk_notebook_insert_page(GTK_NOTEBOOK(editor->options_box), brushinputs, label, 1);
//  gtk_notebook_insert_page(GTK_NOTEBOOK(editor->options_box), brushicon, label, 2);
  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), priv_widget, gtk_label_new(_("Basic")), 0);
  gtk_notebook_insert_page(GTK_NOTEBOOK(options_box), editor_widget, gtk_label_new("Details"), 1);
//  gtk_notebook_set_current_page (GTK_NOTEBOOK(options_box), 1);
#if 0
  GtkWidget*options_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  gtk_widget_show (options_box);
  g_print("MYPAINTOPTIONSGUI::add PRIV_WIDGET=%lx\n", (gulong)(gpointer)priv_widget);
  gtk_box_pack_start(GTK_BOX(options_box), priv_widget, TRUE, TRUE, 2);  
  g_print("MYPAINTOPTIONSGUI::add EDITOR_WIDGET=%lx\n", (gulong)(gpointer)editor_widget);
  gtk_box_pack_start(GTK_BOX(options_box), editor_widget, TRUE, TRUE, 2);  
#endif
  return options_box;
}

GtkWidget *
gimp_mypaint_options_gui_horizontal (GimpToolOptions *tool_options)
{
  MypaintOptionsGUIPrivate* priv = new MypaintOptionsGUIPrivate(tool_options, true);
  return priv->create();
}

}; // extern "C"
