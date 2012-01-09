#ifndef __MYPAINTBRUSH_BRUSHSETTINGS_H__
#define __MYPAINTBRUSH_BRUSHSETTINGS_H__

#include <glib.h>

struct _MyPaintBrushInputSettings {
  gchar   *name;
  gint    index;
  gfloat  hard_minimum;
  gfloat  soft_minimum;
  gfloat  normal;
  gfloat  soft_maximum;
  gfloat  hard_maximum;
  gchar  *displayed_name;
  gchar  *tooltip;  
};
typedef struct _MyPaintBrushInputSettings MyPaintBrushInputSettings;


struct _MyPaintBrushSettings {
  gchar    *internal_name;
  gint      index;
  gchar    *displayed_name;
  gboolean  constant;
  gfloat    minimum;
  gfloat     default_value;
  gfloat    maximum;
  gchar    *tooltip;
};
typedef struct _MyPaintBrushSettings MyPaintBrushSettings;

struct _MyPaintBrushSettingMigrate {
  gchar *old_name;
  gchar *new_name;
  gfloat (*transform)(gfloat y);
};
typedef struct _MyPaintBrushSettingMigrate MyPaintBrushSettingMigrate;

GList *mypaint_brush_get_brush_settings (void);
GList *mypaint_brush_get_input_settings (void);
GHashTable *mypaint_brush_get_brush_settings_dict (void);
GHashTable *mypaint_brush_get_input_settings_dict (void);
GHashTable *mypaint_brush_get_setting_migrate_dict (void);
#endif
