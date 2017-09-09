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
/// Class RESTD

void
RESTD::handle_request(SoupServer*        server,
                      SoupMessage*       msg,
                      const char*        path,
                      GHashTable*        queries,
                      SoupClientContext* context)
{
  g_print("RESTD::handle_request: %s\n", path);
  router.dispatch(path, NULL, gimp, msg, context);
}

RESTD&
RESTD::route(const gchar* path, RESTResourceFactory* factory) {
  router.route(path, Delegators::delegator(factory, &RESTResourceFactory::handle_request));
  return *this;
}

RESTD&
RESTD::route(const gchar* path, RESTD::Delegator* d) {
  router.route(path, d);
  return *this;
}


/////////////////////////////////////////////////////////////////////////////
/// Class RESTDResourceFactory

void
RESTResourceFactory::handle_request(RESTD::Router::Matched* matched,
                                    Gimp* gimp,
                                    SoupMessage* msg,
                                    SoupClientContext* context)
{
  RESTResource* res = create(gimp, matched, msg, context);
  res->handle();
}


/////////////////////////////////////////////////////////////////////////////
/// Class RESTDResource

void RESTResource::handle() {
  if (strcmp(message->method, SOUP_METHOD_GET) == 0) {
    get();
  } else if (strcmp(message->method, SOUP_METHOD_PUT) == 0) {
    put();
  } else if (strcmp(message->method, SOUP_METHOD_POST) == 0) {
    post();
  } else if (strcmp(message->method, SOUP_METHOD_DELETE) == 0) {
    del();
  }
  delete this;
}


GBytes*
RESTResource::req_body_data()
{
  auto msg  = GLib::ref(message);
  GBytes* bytes;
  g_object_get(msg, "request-body-data", &bytes, NULL);
  return bytes;
}


SoupMessageBody*
RESTResource::req_body()
{
  auto msg  = GLib::ref(message);
  SoupMessageBody* result;
  g_object_get(msg, "request-body", &result, NULL);
  return result;
}


SoupMessageHeaders*
RESTResource::req_headers()
{
  SoupMessageHeaders* header;
  g_object_get(message, "request-headers", &header, NULL);
  return header;
}


JSON::INode
RESTResource::req_body_json()
{
  GLib::Object<JsonParser> parser      = json_parser_new();
  GBytes*                  bytes       = req_body_data();
  gsize                    size;
  gchar*                   content_str = (gchar*)g_bytes_get_data(bytes, &size);
  json_parser_load_from_data(parser, content_str, -1, NULL);
  JSON::INode json = json_parser_get_root(parser);
  g_print("req_body_json: json=%s\n", json_to_string(json, TRUE));
  return json;
}


void
RESTResource::make_json_response(gint code, JSON::INode node)
{
  auto imessage = GLib::ref(message);
  GLib::CString result_text = json_to_string(node, FALSE);
  imessage [soup_message_set_response] ("application/json; charset=utf-8", SOUP_MEMORY_COPY,
                                        (const gchar*)result_text, strlen(result_text));
  imessage.set("status-code", code);
}

void
RESTResource::make_error_response(gint code, const gchar* msg)
{
  auto root = JSON::build_object([&](auto it) {
    it["error"] = msg;
  });
  make_json_response(code, root);
}
