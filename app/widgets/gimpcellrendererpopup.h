/* GIMP-painter
 * GimpCellRendererPopup
 * Copyright (C) 2016  seagetch
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

#ifndef __GIMP_CELL_RENDERER_POPUP_H__
#define __GIMP_CELL_RENDERER_POPUP_H__

#ifdef __cplusplus
extern "C" {
#endif

#define GIMP_TYPE_CELL_RENDERER_POPUP            (gimp_cell_renderer_popup_get_type ())
#define GIMP_CELL_RENDERER_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_CELL_RENDERER_POPUP, GimpCellRendererPopup))
#define GIMP_CELL_RENDERER_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_CELL_RENDERER_POPUP, GimpCellRendererPopupClass))
#define GIMP_IS_CELL_RENDERER_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_CELL_RENDERER_POPUP))
#define GIMP_IS_CELL_RENDERER_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_CELL_RENDERER_POPUP))
#define GIMP_CELL_RENDERER_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_CELL_RENDERER_POPUP, GimpCellRendererPopupClass))


typedef struct _GimpCellRendererPopupClass GimpCellRendererPopupClass;

struct _GimpCellRendererPopup
{
  GtkCellRenderer   parent_instance;
};

struct _GimpCellRendererPopupClass
{
  GtkCellRendererClass  parent_class;
  void (* clicked) (GObject *object, GObject* widget, const gchar* path, GdkRectangle* rectangle);
};


GType             gimp_cell_renderer_popup_get_type (void) G_GNUC_CONST;

GtkCellRenderer * gimp_cell_renderer_popup_new      (void);
void              gimp_cell_renderer_popup_clicked  (GimpCellRendererPopup* popup,
                                                     GObject *widget,
                                                     const gchar* path,
                                                     GdkRectangle* cell_area);
#ifdef __cplusplus
}

class CellRendererPopupInterface {
public:
  static GtkCellRenderer*            new_instance   ();
  static CellRendererPopupInterface* cast           (gpointer obj);
  static bool                        is_instance    (gpointer obj);
  virtual void                       clicked        (GObject* widget,
                                                     const gchar* path,
                                                     GdkRectangle* cell_area) = 0;
};

#include "base/glib-cxx-types.hpp"
__DECLARE_GTK_CLASS__(GimpCellRendererPopup, GIMP_TYPE_CELL_RENDERER_POPUP);
#endif

#endif /* __GIMP_CELL_RENDERER_POPUP_H__ */
