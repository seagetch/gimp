/*
 * gimpeditor-private.h
 *
 *  Created on: 2017/09/17
 *      Author: seagetch
 */

#ifndef APP_WIDGETS_GIMPEDITOR_PRIVATE_H_
#define APP_WIDGETS_GIMPEDITOR_PRIVATE_H_

struct _GimpEditorPrivate
{
  GimpMenuFactory *menu_factory;
  gchar           *menu_identifier;
  GimpUIManager   *ui_manager;
  gchar           *ui_path;
  gpointer         popup_data;

  gboolean         show_button_bar;
  GtkWidget       *name_label;
  GtkWidget       *button_box;
};

GtkIconSize     gimp_editor_ensure_button_box   (GimpEditor     *editor,
                                                 GtkReliefStyle *button_relief);
void            gimp_editor_button_extended_actions_free (GList *actions);
void            gimp_editor_button_extended_clicked (GtkWidget       *button,
                                                     GdkModifierType  mask,
                                                     gpointer         data);

typedef struct
{
  GdkModifierType  mod_mask;
  GtkAction       *action;
} ExtendedAction;

#endif /* APP_WIDGETS_GIMPEDITOR_PRIVATE_H_ */
