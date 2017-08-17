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

#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-bridge.hpp"
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "presets-enums.h"

}
#include "presets/gimpjsonresource.h"
#include "layer-preset.h"

using namespace GLib;

////////////////////////////////////////////////////////////////////////////
// LayerPrestConfig

LayerPresetConfig::LayerPresetConfig() : PresetConfig()
{ 
}


LayerPresetConfig::~LayerPresetConfig() 
{
  GimpDataFactory* factory = get_data_factory();
  if (factory)
    gimp_data_factory_data_free (factory);
}


const gchar*
LayerPresetConfig::get_readonly_path_prop_name ()
{
  return "layer-presets-path";
}


const gchar*
LayerPresetConfig::get_writable_path_prop_name ()
{
  return "layer-presets-path-writable";
}


const gchar*
LayerPresetConfig::get_readonly_default_paths  ()
{
  static const gchar* path = gimp_config_build_data_path ("layer-presets");
  return path;
}


const gchar*
LayerPresetConfig::get_writable_default_paths  ()
{
  static const gchar* path = gimp_config_build_writable_path ("layer-presets");
  return path;
}


////////////////////////////////////////////////////////////////////////////
// LayerPrestConfig::startup code
static LayerPresetConfig* instance = NULL;

PresetConfig*
LayerPresetConfig::config()
{
  if (!instance)
    instance = new LayerPresetConfig();
  return instance;
}
