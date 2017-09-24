/*
 * gimpeditor-cxx.h
 *
 *  Created on: 2017/09/17
 *      Author: seagetch
 */

#ifndef APP_WIDGETS_GIMPEDITOR_CXX_H_
#define APP_WIDGETS_GIMPEDITOR_CXX_H_

#ifdef __cplusplus
extern "C" {
#endif

GtkWidget *
gimp_editor_add_dropdown (GimpEditor  *editor,
                          const gchar *stock_id,
                          const gchar *tooltip,
                          const gchar *help_id,
                          const gchar *list_id,
                          GCallback    callback,
                          GCallback    extended_callback,
                          gpointer     callback_data);

GtkWidget *
gimp_editor_add_action_dropdown (GimpEditor  *editor,
                                 const gchar *list_id,
                                 const gchar *group_name,
                                 const gchar *action_name,
                                 ...);

#ifdef __cplusplus
};
#endif

#endif /* APP_WIDGETS_GIMPEDITOR_CXX_H_ */
