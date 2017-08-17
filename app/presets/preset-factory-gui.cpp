/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpPresetGui
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
__DECLARE_GTK_CLASS__(GObject, G_TYPE_OBJECT);
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"
#include "libgimpmath/gimpmath.h"

#ifdef G_OS_WIN32
#include "libgimpbase/gimpwin32-io.h"
#endif

#include "presets-enums.h"

#include "core/gimp.h"

#include "core/gimpcontext.h"
#include "core/gimpcontainer.h"
#include "core/gimpdata.h"

#include "gimp-intl.h"

#include "core/gimpparamspecs.h"
#include "config/gimpcoreconfig.h"

#include "widgets/widgets-types.h"
#include "widgets/gimpdialogfactory.h"
}
#include "presets/gimpjsonresource.h"
#include "preset-factory-gui.h"

#include "layer-preset-gui.h"


using namespace GLib;

extern GLib::GClassWrapper<UseCStructs<GimpBaseConfig, GimpCoreConfig> > core_class;

////////////////////////////////////////////////////////////////////////////
// PresetGuiConfig

GObject*
PresetGuiConfig::create_view (GObject* context,
                           gint view_size,
                           GObject* menu_factory)
{
  return NULL;
}


GObject*
PresetGuiConfig::create_editor ()
{
  return NULL;
}


PrefsDialogInfo*
PresetGuiConfig::prefs_dialog_info ()
{
  return NULL;
}


////////////////////////////////////////////////////////////////////////////
// PresetGuiFactory

static PresetGuiFactory* singleton = NULL;
PresetGuiFactory* get_preset_factory_gui() {
  if (!singleton)
    singleton = new PresetGuiFactory();
  return singleton;
}

void PresetGuiFactory::entry_point(Gimp* gimp)
{
  g_signal_connect_delegator (G_OBJECT(gimp), "initialize", Delegators::delegator(this, &PresetGuiFactory::on_initialize));
  g_signal_connect_delegator (G_OBJECT(gimp), "restore",    Delegators::delegator(this, &PresetGuiFactory::on_restore));
  g_signal_connect_delegator (G_OBJECT(gimp), "exit",       Delegators::delegator(this, &PresetGuiFactory::on_exit));

  GimpCoreConfigClass* klass = GIMP_CORE_CONFIG_CLASS (g_type_class_ref(GIMP_TYPE_CORE_CONFIG));

  register_config( LayerPresetGuiConfig::config() );

  registry().each([&](auto key, auto it) {
    it->extend(klass);
  });
  g_type_class_unref(klass);
}


GArray*
PresetGuiFactory::prefs_entry_point ()
{
  GArray* result = g_array_new(FALSE, TRUE, sizeof(PrefsDialogInfo));
  auto array = ref<PrefsDialogInfo>(result);

  registry().each([&](auto key, auto it) {
    PrefsDialogInfo* info = it->prefs_dialog_info();
    if (info) {
      g_print("PresetGuiFactory::prefs_entry_point: add %p\n", info);
      array.append( *info );
    }
  });
  g_print("PresetGuiFactory::prefs_entry_point: end\n");
  return result;
}


void
PresetGuiFactory::on_initialize (Gimp               *gimp,
                                    GimpInitStatusFunc  status_callback)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = this->create_data_factory(gimp, it);
    it->set_data_factory(factory);
  });
}


void
PresetGuiFactory::on_restore (Gimp               *gimp,
                                 GimpInitStatusFunc  status_callback)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = it->get_data_factory();
    gimp_data_factory_data_init (factory, gimp->user_context,
                                 gimp->no_data);
  });

  // Dialog initialization.

  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = it->get_data_factory();
    GimpDialogFactory* gimp_dialog_factory_get_singleton();
#if 0
    gimp_dialog_factory_register_entry (GimpDialogFactory    *factory,
                                        const gchar          *identifier,
                                        const gchar          *name,
                                        const gchar          *blurb,
                                        const gchar          *stock_id,
                                        const gchar          *help_id,
                                        GimpDialogNewFunc     new_func,
                                        GimpDialogRestoreFunc restore_func,
                                        gint                  view_size,
                                        gboolean              singleton,
                                        gboolean              session_managed,
                                        gboolean              remember_size,
                                        gboolean              remember_if_open,
                                        gboolean              hideable,
                                        gboolean              image_window,
                                        gboolean              dockable);
#endif
  });
}


gboolean
PresetGuiFactory::on_exit(Gimp *gimp,
                             gboolean force)
{
  registry().each([&](auto key, auto it) {
    GimpDataFactory* factory = it->get_data_factory();
    gimp_data_factory_data_save (factory);
  });
  return FALSE;
}


void
preset_factory_gui_entry_point (Gimp* gimp)
{
  get_preset_factory_gui()->entry_point(gimp);
}


GArray*
preset_factory_gui_prefs_entry_point ()
{
  return get_preset_factory_gui()->prefs_entry_point();
}
