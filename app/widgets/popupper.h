/*
 * popupper.h
 *
 *  Created on: 2017/03/24
 *      Author: seagetch
 */

#ifndef APP_WIDGETS_POPUPPER_H_
#define APP_WIDGETS_POPUPPER_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*GimpPopupCreateViewCallback) (GtkWidget *button, GtkWidget** result, gpointer data);
typedef void (*GimpPopupCreateViewCallbackExt) (GtkWidget *button, GtkWidget** result, GObject *config, gpointer data);


#ifdef __cplusplus
}; //namespace

typedef Delegators::Delegator<void(GtkWidget*, GtkWidget**, gpointer)> CreateViewDelegator;
void decorate_popupper(GObject* widget, CreateViewDelegator* delegator);

#endif

#endif /* APP_WIDGETS_POPUPPER_H_ */
