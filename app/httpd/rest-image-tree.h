/*
 * rest-image-tree.h
 *
 *  Created on: 2017/09/04
 *      Author: seagetch
 */

#ifndef APP_HTTPD_REST_IMAGE_TREE_H_
#define APP_HTTPD_REST_IMAGE_TREE_H_

#include "httpd.h"

class RESTImageTree : public RESTResource {
public:
  RESTImageTree(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) :
    RESTResource(gimp, matched, msg, context) { }
  virtual void get();
  virtual void put();
  virtual void post();
  virtual void del();
};

class RESTImageTreeFactory : public RESTResourceFactory {
public:
  virtual RESTResource* create(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context) {
    return new RESTImageTree(gimp, matched, msg, context);
  }
};

#endif /* APP_HTTPD_REST_IMAGE_TREE_H_ */
