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

typedef Delegator::Delegator<void (*)(GObject*,GtkWidget**)> PopupCreateViewDelegator;

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

#endif  /*  __GIMP_TOOL_OPTIONS_GUI_CXX_HPP__  */
