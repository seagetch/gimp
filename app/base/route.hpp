#ifndef __ROUTE_HPP__
#define __ROUTE_HPP__

#include "glib-cxx-utils.hpp"

/////////////////////////////////////////////////////////////////////////////
/// Class Route
///  Manage mapping from URI to handler delegators.
template<typename Ret, typename... Args>
class Route {
public:
  struct Matched {
    int applied_rule_id;
    char** path;
    GHashTableHolder<gchar*, gchar*> data;
    Matched() : applied_rule_id(0), path(NULL), data(g_hash_table_new(g_str_hash, g_str_equal)) {}
    Matched(const Matched& src) : applied_rule_id(src.applied_rule_id), 
                                  path(g_strdupv((gchar**)src.path)), 
                                  data(g_hash_table_ref((GHashTable*)src.data.ptr())) {}
    ~Matched() {
      if (path)
        g_strfreev(path);
      path = NULL;
    }
  };
  typedef bool rule_handler(Matched*, Args...);
  typedef Delegator::Delegator<Ret, Matched*, Args...> rule_delegator;

protected:
  struct Rule {
    struct TokenRule { virtual bool test(const gchar* token, Matched& result) = 0; };
    struct Match: TokenRule {
      String token;
      Match(const gchar* name) : token(g_strdup(name)) {}
      Match(const Match& src) : token(g_strdup(src.token)) {}
      virtual bool test(const gchar* tested_token, Matched& result) {
        return strcmp(token, tested_token) == 0;
      }
    };
    struct Name: TokenRule {
      String name;
      Name(const gchar* n) : name(g_strdup(n)) {}
      Name(const Name& src) : name(g_strdup(src.name)) {}
      virtual bool test(const gchar* tested_token, Matched& result) {
        g_hash_table_insert(result.data, name, g_strdup(tested_token));
        return true;
      }
    };
    struct Select: TokenRule {
      String name;
      gchar** candidates;
      Select(const gchar* n, gchar** c) : name(g_strdup(n)), candidates(g_strdupv(c)) {}
      Select(const Select& src) : name(g_strdup(src.name)), candidates(g_strdupv(src.candidates)) {}
      ~Select() {
        if (candidates)
          g_strfreev(candidates);
      }
      virtual bool test(const gchar* tested_token, Matched& result) {
        for (gchar** next_token = candidates; *next_token; next_token ++) {
          if (strcmp(*next_token, tested_token) == 0) {
            g_hash_table_insert(result.data, name, g_strdup(tested_token));
            return true;
          }
        }
        return false;
      }
    };
    std::vector<TokenRule*> rules;
    rule_delegator* handler;
    
    Rule(const gchar* path, rule_delegator* h) {
      handler = h;
      g_print("Rule::constructor parse %s\n", path);
      parse_rule(path);
    }
    
    void parse_rule(const gchar* path) {
      ScopedPointer<gchar*, void (gchar**), g_strfreev> rule_path(g_strsplit(path, "/", 256));
      for (gchar** token = rule_path.ptr(); *token; token ++) {
        if (!*token || strlen(*token) == 0)
          continue;
        g_print("Rule::constructor parse: transform %s\n", *token);
        switch ((*token)[0]) {
        case '<': {
            gchar* word_term = strstr(&((*token)[1]),">");
            if (word_term) {
              String varname = strndup(&((*token)[1]), word_term - &((*token)[1]));

              gchar* type_term = strstr(varname.ptr(), ":");
              if (type_term) { // Have type declaration or selection
                String type = strndup(varname.ptr(), type_term - varname.ptr());
                String subname = strdup(type_term + 1);
                ScopedPointer<gchar*, void (gchar**), g_strfreev> candidates(g_strsplit(type.ptr(), "|", 256));
                g_print("register Select(%s)\n", subname.ptr());
                rules.push_back(new Select(subname.ptr(), candidates.ptr()));
              } else { // Have no type declaration, assume string type.
                g_print("register Name(%s)\n", varname.ptr());
                rules.push_back(new Name(varname.ptr()));
              }
            }
            break;
          }
        default: {
            rules.push_back(new Match(*token));
            g_print("register Match(%s)\n", *token);
            break;
          }
        }
      };
    };
    
    ~Rule() {
      for (auto i = rules.begin(); i != rules.end(); i ++) {
        delete *i;
      }
      rules.clear();
      if (handler)
        delete handler;
      handler = NULL;
    }
    
    Matched test(const gchar** uri, const gchar* data) {
      Matched result;
      result.path = NULL;
      if (rules.size() == 0)
        return result;
      
      gchar** tested_token = (gchar**)uri;
      for (auto i = rules.begin(); i != rules.end(); i ++) {
        const gchar* token = *tested_token;
        while (token && strlen(token) == 0)
          token = *(++tested_token);
        if (!token) { // Tested URI is short.
          return result;
        }
        if (!(*i)->test(token, result))
          return result;
        tested_token ++;
      }
      if (*tested_token)
        return result;
      result.path = g_strdupv((gchar**)uri);
      return result;
    }
    
  };
  std::vector<Route::Rule*> rules;
  
public:
  Route() {}
  ~Route() {
    for (auto i = rules.begin(); i != rules.end(); i ++) {
      delete *i;
    }
    rules.clear();
  }
  int route(const gchar* route, rule_delegator* handler) {
    rules.push_back(new Rule(route, handler));
    return rules.size() - 1;
  };
  void block(const int id) {
    rules.at(id)->handler->disable();
  }
  void unblock(const int id) {
    rules.at(id)->handler->enable();
  }
  Ret dispatch(const gchar* uri, const gchar* data, Ret default_value, Args... args);
};

template<typename Ret, typename... Args>
Ret Route<Ret, Args...>::dispatch(const gchar* uri, const gchar* data, Ret default_value, Args... args) {
  ScopedPointer<gchar*, void (gchar**), g_strfreev> tested_path = g_strsplit(uri, "/", 256);
  int rule_id = 0;
  for (auto i = rules.begin(); i != rules.end(); i ++, rule_id ++) {
    Matched route = (*i)->test((const gchar**)tested_path.ptr(), data);
    if (route.path) {
      route.applied_rule_id = rule_id;
      return (*i)->handler->emit(&route, args...);
    }
  }
  return default_value;
}

#if 0
template<typename... Args>
void Route<void, Args...>::dispatch(const gchar* uri, const gchar* data, Args... args) {
  ScopedPointer<gchar*, void (gchar**), g_strfreev> tested_path = g_strsplit(uri, "/", 256);
  for (auto i = rules.begin(); i != rules.end(); i ++) {
    Matched route = (*i)->test((const gchar**)tested_path.ptr(), data);
    if (route.path) {
      (*i)->handler->emit(&route, args...);
      break;
    }
  }
}
#endif

#endif
