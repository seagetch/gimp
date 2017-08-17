/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPreset
 * Copyright (C) 2017 seagetch <sigetch@gmail.com>
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

#ifndef __PRESET_FACTORY_GUI_H__
#define __PRESET_FACTORY_GUI_H__

#include "preset-factory.h"

#ifdef __cplusplus
extern "C" {
#endif

void       preset_factory_gui_entry_point       (Gimp* gimp);
GArray*    preset_factory_gui_prefs_entry_point (void);


#ifdef __cplusplus
};// extern "C"

/////////////////////////////////////////////////////////////////////
class PresetGuiConfig : virtual public PresetConfig
{
public:
  PresetGuiConfig() : PresetConfig() { };
  virtual ~PresetGuiConfig() { };

  ////////////////////////////////////////////
  // GUI Interface

  virtual GObject*          create_view                 (GObject* context,
                                                         gint view_size,
                                                         GObject* menu_factory);
  virtual GObject*          create_editor               ();
  virtual PrefsDialogInfo*  prefs_dialog_info           ();

};

/////////////////////////////////////////////////////////////////////
class PresetGuiFactory : virtual public PresetFactory
{
public:
  void             entry_point             (Gimp* gimp);
  GArray*          prefs_entry_point       ();
  virtual void     on_initialize          (Gimp               *gimp,
                                           GimpInitStatusFunc  status_callback);
  virtual void     on_restore             (Gimp               *gimp,
                                           GimpInitStatusFunc  status_callback);
  virtual gboolean on_exit                (Gimp               *gimp,
                                           gboolean            force);

};

PresetGuiFactory* get_preset_factory_gui(void);

#endif // __cplusplus

#endif /* __PRESET_FACTORY_GUI_H__ */
