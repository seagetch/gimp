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

#ifndef __GIMP_TOOL_OPTIONS_GUI_CXX_HPP__
#define __GIMP_TOOL_OPTIONS_GUI_CXX_HPP__

#include "base/delegators.hpp"
using namespace Delegators;
using namespace GLib;

typedef Delegators::Delegator<void, GObject*,GtkWidget**> PopupCreateViewDelegator;

GtkWidget * gimp_tool_options_button_with_popup (GtkWidget                *label_widget,
                                                 PopupCreateViewDelegator* create_view);

template<typename CXXType>
GtkWidget * gimp_tool_options_button_with_popup (GtkWidget                *label_widget,
                                                 PopupCreateViewDelegator* create_view,
                                                 CXXType*                  object) {
  GtkWidget* result = gimp_tool_options_button_with_popup (label_widget, create_view);
  g_object_set_cxx_object( G_OBJECT(result), "behaviour", object);
  return result;
}                                                

class ComplexBindAction {
protected:
  GObject* receiver;

  template<typename Delegator>
  void bind_to_full(GObject* target, const gchar* signal_name, Delegator* delegator) {
    CString object_id(g_strdup_printf("behavior-%s-action", signal_name));
    g_signal_connect_delegator (G_OBJECT(target), signal_name, delegator);
    g_object_set_cxx_object(G_OBJECT(target), (const gchar*)object_id, this);
  };

public:
  ComplexBindAction(GObject* receiver_) : receiver(receiver_) {
  };
  virtual ~ComplexBindAction() {}
  
};

class SimpleBindAction : public ComplexBindAction {
protected:
  virtual void emit(GObject* event_target) {};

  void bind_to(GObject* target, const gchar* signal_name) {
    bind_to_full(target, signal_name, Delegators::delegator(this, &SimpleBindAction::emit));
  };

public:
  SimpleBindAction(GObject* target_, GObject* receiver_, const gchar* signal_name) : ComplexBindAction(receiver_) {
    bind_to(target_, signal_name);
  };
  ~SimpleBindAction() {}
  
};

class ToggleWidgetAction : public SimpleBindAction {
public:
  ToggleWidgetAction(GObject* target, GObject* receiver) : SimpleBindAction(target, receiver, "clicked") {};
  ~ToggleWidgetAction() {}

  void emit(GObject* event_target) {
    gboolean visible = gtk_widget_get_visible(GTK_WIDGET(receiver));
    gtk_widget_set_visible(GTK_WIDGET(receiver), !visible);
  };
};

class PageRemindAction : public ComplexBindAction {
protected:
  guint last_page;
  
  void emit (GObject *object, GtkWidget* page, guint page_num) {
    last_page = page_num;
    g_print("PageRemindAction::switch page=%d\n", page_num);
  }
public:
  PageRemindAction(GObject* receiver) : 
    ComplexBindAction(receiver), last_page(0)
  {
  }

  void bind_to(GtkNotebook* notebook) 
  {
    g_return_if_fail (GTK_IS_NOTEBOOK(notebook));
    g_return_if_fail (gtk_notebook_get_n_pages(notebook) >= last_page);
    gtk_notebook_set_current_page(notebook, last_page);
    g_signal_connect_delegator(G_OBJECT(notebook), "switch-page", Delegators::delegator(this, &PageRemindAction::emit));
    g_print("PageRemindAction::set page=%d\n", last_page);
  };
};


#endif  /*  __GIMP_TOOL_OPTIONS_GUI_CXX_HPP__  */
