/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * httpd
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
#include "base/scopeguard.hpp"
#include "base/glib-cxx-utils.hpp"

#include "httpd.h"

extern "C" {
#include <glib.h>
#include <string.h>
};
#include <functional>

#include "core/core-types.h"

/////////////////////////////////////////////////////////////////////////////
/// Class HTTPD
///  Base class of http server
void HTTPD::run() {
  GError* error;
  auto server = Soup::ref( soup_server_new(SOUP_SERVER_SERVER_HEADER, "gimp", NULL) );
  server [soup_server_listen_local] (port(), (SoupServerListenOptions)0, &error);
  server.add_handler("/", Delegators::delegator(this, &HTTPD::handle_request));
}


/////////////////////////////////////////////////////////////////////////////
/// Class Route
///  Manage mapping from URI to handler delegators.


void
RESTD::handle_request(SoupServer*        server,
                              SoupMessage*       msg,
                              const char*        path,
                              GHashTable*        queries,
                              SoupClientContext* context)
{
  g_print("RESTD::handle_request: %s\n", path);
  router.dispatch(path, NULL, msg, context);
}

RESTD&
RESTD::route(const gchar* path, RESTD::Delegator* delegator) {
  router.route(path, delegator);
  return *this;
}

