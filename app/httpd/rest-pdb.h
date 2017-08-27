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

#ifndef APP_HTTPD_REST_PDB_H_
#define APP_HTTPD_REST_PDB_H_

#include "httpd.h"

class RESTPDB : public RESTResource {
public:
  RESTPDB(RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) :
    RESTResource(matched, msg, context) { }
  virtual void get();
  virtual void put();
  virtual void post();
  virtual void del();
};

class RESTPDBFactory : public RESTResourceFactory {
public:
  virtual RESTResource* create(RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) {
    return new RESTPDB(matched, msg, context);
  }
};

#endif /* APP_HTTPD_REST_PDB_H_ */
