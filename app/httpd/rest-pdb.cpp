/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * rest-pdb
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
#include <gegl.h>

#include "core/core-types.h"
#include "pdb/pdb-types.h"

#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimpdrawable.h"
#include "core/gimpitem.h"
}

#include "rest-pdb.h"
#include "pdb/pdb-cxx-utils.hpp"


void RESTPDB::get()
{

}


void RESTPDB::put()
{

}


void RESTPDB::post()
{
  JSON::INode json = req_body_json();
  if (!json.is_object())
    return;

  try {
    auto ctx_node = json["context"];
    GLib::IObject<GimpImage> image;

    if (ctx_node.is_object()) {

    } else {
      return;
    }

    {

      const gchar* proc_name = json["name"];
      auto arg_node = json["arguments"];



      if (arg_node.is_array()) {
        "";
      }
    }

  } catch(JSON::INode::InvalidType e) {
  } catch(JSON::INode::InvalidIndex e) {

  }
}


void RESTPDB::del()
{

}
