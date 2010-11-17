/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimppopupbutton.h
 * Copyright (C) 2003-2005 Michael Natterer <mitch@gimp.org>
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

#ifndef __GIMP_POPUP_BUTTON_H__
#define __GIMP_POPUP_BUTTON_H__

#include "libgimpwidgets/gimpbutton.h"


#define GIMP_TYPE_POPUP            (gimp_popup_get_type ())
#define GIMP_POPUP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POPUP, GimpPopup))
#define GIMP_POPUP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POPUP, GimpPopupClass))
#define GIMP_IS_POPUP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POPUP))
#define GIMP_IS_POPUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POPUP))
#define GIMP_POPUP_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POPUP, GimpPopupClass))


typedef struct _GimpPopupClass  GimpPopupClass;
typedef struct _GimpPopup       GimpPopup;

struct _GimpPopup
{
  GtkWindow            parent_instance;
/*
  GimpContainer       *container;
  GimpContext         *orig_context;
  GimpContext         *context;

  GimpViewType         view_type;
*/
  gint                 default_view_size;
  gint                 view_size;
  gint                 view_border_width;

  GtkWidget           *frame;
  GtkWidget           *view;
/*
  GimpContainerEditor *editor;

  GimpDialogFactory   *dialog_factory;
  gchar               *dialog_identifier;
  gchar               *dialog_stock_id;
  gchar               *dialog_tooltip;
*/
};

struct _GimpPopupClass
{
  GtkWindowClass  parent_instance;

  void (* cancel)  (GimpPopup *popup);
  void (* confirm) (GimpPopup *popup);
};


GType       gimp_popup_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_popup_new      (GtkWidget *view);

void        gimp_popup_show     (GimpPopup *popup,
                                  GtkWidget          *widget);

gint     gimp_popup_get_view_size (GimpPopup *popup);
void    gimp_popup_set_view_size (GimpPopup *popup,
                                   gint       view_size);
void     gimp_popup_set_view  (GimpPopup *popup, GtkWidget *widget);



G_BEGIN_DECLS

#define GIMP_TYPE_POPUP_BUTTON            (gimp_popup_button_get_type ())
#define GIMP_POPUP_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GIMP_TYPE_POPUP_BUTTON, GimpPopupButton))
#define GIMP_POPUP_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GIMP_TYPE_POPUP_BUTTON, GimpPopupButtonClass))
#define GIMP_IS_POPUP_BUTTON(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GIMP_TYPE_POPUP_BUTTON))
#define GIMP_IS_POPUP_BUTTON_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GIMP_TYPE_POPUP_BUTTON))
#define GIMP_POPUP_BUTTON_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GIMP_TYPE_POPUP_BUTTON, GimpPopupButtonClass))


typedef struct _GimpPopupButtonClass GimpPopupButtonClass;
typedef struct _GimpPopupButton      GimpPopupButton;

struct _GimpPopupButton
{
  GimpButton         parent_instance;

  GimpViewType       popup_view_type;
  gint               popup_view_size;

  gint               button_view_size;
  gint               view_border_width;

  GtkWidget         *label;
};

struct _GimpPopupButtonClass
{
  GimpButtonClass  parent_class;
};


GType       gimp_popup_button_get_type (void) G_GNUC_CONST;

GtkWidget * gimp_popup_button_new      (GtkWidget *label);

gint         gimp_popup_button_get_view_size (GimpPopupButton *button);
void         gimp_popup_button_set_view_size (GimpPopupButton *button,
                                                 gint                view_size);
GtkWidget* gimp_popup_button_create_view (GimpPopupButton *button);

G_END_DECLS

#endif  /*  __GIMP_POPUP_BUTTON_H__  */
