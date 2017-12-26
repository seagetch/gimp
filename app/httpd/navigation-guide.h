/*
 * navigation-guide.h
 *
 *  Created on: 2017/12/26
 *      Author: seagetch
 */

#ifndef APP_HTTPD_NAVIGATION_GUIDE_H_
#define APP_HTTPD_NAVIGATION_GUIDE_H_

#include "httpd.h"

class NavigationGuideFactory : public RESTResourceFactory {
public:
  virtual RESTResource* create(Gimp* gimp, RESTD::Router::Matched* matched, SoupMessage* msg, SoupClientContext* context);
};

#endif /* APP_HTTPD_NAVIGATION_GUIDE_H_ */
