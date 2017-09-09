/*
 * rest-image-tree.h
 *
 *  Created on: 2017/09/04
 *      Author: seagetch
 */

#ifndef APP_HTTPD_REST_IMAGE_TREE_H_
#define APP_HTTPD_REST_IMAGE_TREE_H_

#include "httpd.h"

class RESTImageTreeFactory : public RESTResourceFactory {
public:
  virtual RESTResource* create(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context);
};

#endif /* APP_HTTPD_REST_IMAGE_TREE_H_ */
