/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * navigation-guide
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

#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-bridge.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"

#include <gegl.h>
#include <gio/gio.h>

#include "libgimpcolor/gimpcolor.h" // GIMP_TYPE_RGB
#include "libgimpbase/gimpbase.h"   // GIMP_TYPE_PARASITE

#include "core/core-types.h"
#include "pdb/pdb-types.h"

#include "core/gimpimage.h"
#include "core/gimpimage-new.h"
#include "core/gimplayer.h"
#include "core/gimpdrawable.h"
#include "core/gimpitem.h"
#include "core/gimp.h"
#include "core/gimpcontainer.h"
#include "pdb/gimppdb-query.h"
#include "pdb/gimpprocedure.h"
#include "core/gimpgrouplayer.h"
}
#include "core/gimpfilterlayer.h"
#include "core/gimpclonelayer.h"

#include "gimp-intl.h"

#include "navigation-guide.h"
//#include "pdb/pdb-cxx-utils.hpp"

template<typename F, typename Ret, typename... Args>
static Ret callback(Args... args, gpointer p) {
  F* f = (F*)p;
  return (*f)(args...);
}

template<typename F>
inline void gimp_container_foreach_(GimpContainer* container, F f) {
  gimp_container_foreach(container, GFunc(callback<F, void, GimpItem*>), &f);
}

namespace GLib {
#if 0
template<> inline GType g_type<GimpLayerModeEffects>() { return GIMP_TYPE_LAYER_MODE_EFFECTS; }
template<> inline GType g_type<GimpImageBaseType>() { return GIMP_TYPE_IMAGE_BASE_TYPE; }
};

enum EMethodId { none = 0, info, data, preview };
struct MethodMap {
  const gchar* method_name;
  EMethodId method_id;
};
#endif
};

///////////////////////////////////////////////////////////////////////
// Navigation REST interface

class NavigationGuide : public RESTResource {

public:
  NavigationGuide(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) :
    RESTResource(gimp, matched, msg, context) { }
  virtual void get();
  virtual void put();
  virtual void post();
  virtual void del();
};


RESTResource*
NavigationGuideFactory::create(Gimp* gimp,
                             RESTD::Router::Matched* matched,
                             SoupMessage* msg,
                             SoupClientContext* context)
{
  return new NavigationGuide(gimp, matched, msg, context);
}


void NavigationGuide::get()
{
  const gchar*            path       = matched->data()["**"];

}


void NavigationGuide::put()
{
  const gchar*            path       = matched->data()["**"];
}


void NavigationGuide::post()
{
}


void NavigationGuide::del()
{

}
