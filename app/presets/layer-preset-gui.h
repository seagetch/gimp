/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpJsonResource
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

#ifndef __LAYER_PRESET_GUI_H__
#define __LAYER_PRESET_GUI_H__

#include "preset-factory-gui.h"
#include "layer-preset.h"

class LayerPresetGuiConfig : virtual public LayerPresetConfig, virtual public PresetGuiConfig {
  PrefsDialogInfo info;
public:
  LayerPresetGuiConfig();
  virtual ~LayerPresetGuiConfig();

  virtual GObject*         create_view       (GObject* context,
                                              gint view_size,
                                              GObject* menu_factory);
  virtual GObject*         create_editor     ();
  virtual PrefsDialogInfo* prefs_dialog_info ();

  static PresetGuiConfig* config();

};
#endif /* __LAYER_PRESET_GUI_H__ */
