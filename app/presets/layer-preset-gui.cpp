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

#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-bridge.hpp"
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"
#include <string.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "widgets/widgets-types.h"
#include "widgets/gimpdatafactoryview.h"

#include "presets-enums.h"

#include "gimp-intl.h"
#include "core/gimpcontext.h"
#include "widgets/gimpmenufactory.h"

}
#include "presets/gimpjsonresource.h"
#include "layer-preset-gui.h"

using namespace GLib;

////////////////////////////////////////////////////////////////////////////
// LayerPrestFactoryView

class LayerPresetFactoryView : virtual public ImplBase
{
public:
  LayerPresetFactoryView(GObject* o) : ImplBase(o) {
  }


  ~LayerPresetFactoryView() {

  }

};

extern const char layer_preset_factory_view[] = "GimpLayerPresetFactoryView";
GLib::NewGClass<layer_preset_factory_view, DerivedFrom<GimpDataFactoryView>, LayerPresetFactoryView>
class_instance([](IGClass::IWithClass* with_class){

});

using GClass                     = typename GLib::strip_ref<decltype(class_instance)>::type;
using GimpLayerPresetFactoryView = ::GClass::Instance;

namespace GLib {
template<> class Traits<::GClass::Instance> : public ::GClass::Traits { };
}




////////////////////////////////////////////////////////////////////////////
// LayerPrestGuiConfig

LayerPresetGuiConfig::LayerPresetGuiConfig() :
    LayerPresetConfig()
{
  PrefsDialogInfo i = {
    N_("Layer Presets"), N_("Layer presets Folders"), "folders-layer-presets",
    "",
    N_("Select Layer Presets Folders"),
    get_readonly_path_prop_name(), get_writable_path_prop_name()
  };
  info = i;
}


LayerPresetGuiConfig::~LayerPresetGuiConfig()
{
}
/*
gimp_tool_preset_factory_view_new (GimpViewType      view_type,
                                   GimpDataFactory  *factory,
                                   GimpContext      *context,
                                   gint              view_size,
                                   gint              view_border_width,
                                   GimpMenuFactory  *menu_factory)

typedef GtkWidget * (* GimpDialogNewFunc)     (GimpDialogFactory      *factory,
                                               GimpContext            *context,
                                               GimpUIManager          *ui_manager,
                                               gint                    view_size);
*/
GObject*
LayerPresetGuiConfig::create_view (GObject* context,
                                   gint view_size,
                                   GObject* menu_factory)
{ 
  GimpDataFactory*            factory = get_data_factory();
  GimpLayerPresetFactoryView* factory_view;

  g_return_val_if_fail (GIMP_IS_CONTEXT (context), NULL);
  g_return_val_if_fail (view_size > 0 &&
                        view_size <= GIMP_VIEWABLE_MAX_PREVIEW_SIZE, NULL);
  g_return_val_if_fail (menu_factory == NULL ||
                        GIMP_IS_MENU_FACTORY (menu_factory), NULL);

  factory_view = Traits<GimpLayerPresetFactoryView>::cast(
      g_object_new (Traits<GimpLayerPresetFactoryView>::get_type(),
                    "view-type",         GIMP_VIEW_TYPE_LIST,
                    "data-factory",      factory,
                    "context",           context,
                    "view-size",         view_size,
                    "view-border-width", 0,
                    "menu-factory",      menu_factory,
                    "menu-identifier",   "<LayerPresets>",
                    "ui-path",           "/layer-presets-popup",
                    "action-group",      "layer-presets",
                    NULL));

  gtk_widget_hide (gimp_data_factory_view_get_duplicate_button (GIMP_DATA_FACTORY_VIEW (factory_view)));
  return G_OBJECT(factory_view);
}


GObject*
LayerPresetGuiConfig::create_editor ()
{ 
  return NULL; 
}


PrefsDialogInfo*
LayerPresetGuiConfig::prefs_dialog_info ()
{
  return &info;
}

////////////////////////////////////////////////////////////////////////////
// LayerPrestGuiConfig::startup code
static LayerPresetGuiConfig* instance = NULL;

PresetGuiConfig*
LayerPresetGuiConfig::config()
{
  if (!instance)
    instance = new LayerPresetGuiConfig();
  return instance;
}
