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

#ifndef __HTTPD_H__
#define __HTTPD_H__

#include "base/glib-cxx-utils.hpp"
#include "core/core-types.h"
#include "base/soup-cxx-utils.hpp"
#include "base/json-cxx-utils.hpp"

class HTTPD {
protected:
  Gimp* gimp;

public:
  HTTPD(Gimp* _gimp) : gimp(_gimp) { };
  virtual ~HTTPD() { };

  virtual gint port() = 0;
  virtual void handle_request(SoupServer* server,
                              SoupMessage* msg,
                              const char* path,
                              GHashTable* queries,
                              SoupClientContext* context) = 0;
  void run();
};

class RESTResourceFactory;

class RESTD : public HTTPD {
public:
  using Router = Soup::Router<void, Gimp*, SoupMessage*, SoupClientContext*>;
  using Delegator = Router::rule_delegator;

protected:
  Router router;

public:
  enum { HTTP_PORT = 8920 }; // Mayoi is god ;)

  RESTD(Gimp* gimp) : HTTPD(gimp) { };
  virtual ~RESTD() { };

  virtual gint port() { return HTTP_PORT; }
  virtual void handle_request(SoupServer*        server,
                              SoupMessage*       msg,
                              const char*        path,
                              GHashTable*        queries,
                              SoupClientContext* context);

  RESTD& route(const gchar* path, RESTResourceFactory* factory);
  RESTD& route(const gchar* path, Delegator* d);

  template<typename F>
  static auto delegator(F f) {
    std::function<void (Router::Matched*, Gimp*, SoupMessage*, SoupClientContext*)> func = f;
    return Delegators::delegator(std::move(func));
  }

}; // class RESTD

class RESTResourceFactory;

class RESTResource {
protected:
  friend class RESTResourceFactory;

  Gimp*                   gimp;
  RESTD::Router::Matched* matched;
  SoupMessage*            message;
  SoupClientContext*      context;

  void handle();

  GBytes*             req_body_data();
  SoupMessageBody*    req_body();
  JSON::INode         req_body_json();
  SoupMessageHeaders* req_headers();

public:
  ~RESTResource() {}

  RESTResource(Gimp*                   _gimp,
               RESTD::Router::Matched* m,
               SoupMessage*            msg,
               SoupClientContext*      c) :
    gimp(_gimp), matched(m), message(msg), context(c)
  {
  }

  virtual void get () { }
  virtual void put () { }
  virtual void post() { }
  virtual void del () { }

  void make_json_response(gint code, JSON::Node node);
  void make_error_response(gint code, const gchar* error_msg);
  template<typename... Args>
  void make_error_response(gint code, const gchar* error_msg, Args... args) {
    GLib::CString msg = g_strdup_printf(error_msg, args...);
    make_error_response(code, msg);
  }
};

class RESTResourceFactory {
public:
  void handle_request(RESTD::Router::Matched* matched, Gimp* gimp, SoupMessage* msg, SoupClientContext* context);
  virtual RESTResource* create(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) = 0;
};


#endif /* __HTTPD_H__ */
