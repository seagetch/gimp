#ifndef __HTTPD_H__
#define __HTTPD_H__

#include "base/soup-cxx-utils.hpp"

class HTTPD {
public:
  HTTPD() { };
  virtual ~HTTPD() { };

  virtual gint port() = 0;
  virtual void handle_request(SoupServer* server,
                              SoupMessage* msg,
                              const char* path,
                              GHashTable* queries,
                              SoupClientContext* context) = 0;
  void run();
};


class RESTD : public HTTPD {
public:
  using Router = Soup::Router<void, SoupMessage*, SoupClientContext*>;
  using Delegator = Router::rule_delegator;

protected:
  Router router;

public:
  enum { HTTP_PORT = 8920 }; // Mayoi is god ;)

  RESTD() : HTTPD() { };
  virtual ~RESTD() { };

  virtual gint port() { return HTTP_PORT; }
  virtual void handle_request(SoupServer*        server,
                              SoupMessage*       msg,
                              const char*        path,
                              GHashTable*        queries,
                              SoupClientContext* context);

  RESTD& route(const gchar* path, RESTD::Delegator* delegator);

  template<typename F>
  static auto delegator(F f) {
    std::function<void (Router::Matched*, SoupMessage*, SoupClientContext*)> func = f;
    return Delegators::delegator(std::move(func));
  }

}; // class RESTD

#endif /* __HTTPD_H__ */
