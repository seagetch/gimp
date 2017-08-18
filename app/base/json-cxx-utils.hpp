#ifndef __JSON_CXX_UTILS_HPP__
#define __JSON_CXX_UTILS_HPP__

extern "C" {
#include <json-glib/json-glib.h>
}

#include <iterator>
#include <functional>

#include "base/scopeguard.hpp"
#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-utils.hpp"

namespace Json {

using namespace GLib;
using namespace Delegators;


class Node : public ScopedPointer<JsonNode, void(JsonNode*), json_node_unref>
{
  typedef ScopedPointer<JsonNode, void(JsonNode*), json_node_unref> super;
public:
  Node() : super(NULL) {};
  Node(JsonNode* node) : super(node) {};
  Node(Node&& src) : super(src.obj) { src.obj = NULL; }

  void incref() { json_node_ref(obj); };
  void decref() { json_node_unref(obj); };

  void operator = (JsonNode* src) {
    if (super::obj == src)
      return;
    if (!is_null(super::obj))
      decref();
    super::obj = src;
  };

};

inline auto hold(JsonNode* src) { return Node(src); }

class INode : public Node
{
  typedef Node super;
public:
  INode() : super(NULL) { };
  INode(JsonNode* object) : super(object) {
    super::incref();
  };
  INode(const INode& src) : super (src.obj) {
    super::incref();
  }
  
  bool is_object() const { return JSON_NODE_HOLDS_OBJECT(obj); }
  bool is_array()  const { return JSON_NODE_HOLDS_ARRAY (obj); }
  bool is_value()  const { return JSON_NODE_HOLDS_VALUE (obj); }
  bool is_null()   const { return json_node_is_null (obj); }

  struct InvalidMember { const gchar* name; InvalidMember(const gchar* n) : name(n) {;}; };
  struct InvalidIndex  { const int index; InvalidIndex(const int i) : index(i) {;}; };
  struct InvalidType { const JsonNodeType specified; const JsonNodeType actual; InvalidType(const JsonNodeType s, const JsonNodeType a) : specified(s), actual(a) {;}; };
  struct RuntimeError  { const GError* error; RuntimeError(const GError* e) : error(e) {;}; };
  
  auto operator[](const gchar* prop_name) {
    if (!is_object())
      throw InvalidType(JSON_NODE_OBJECT, JSON_NODE_TYPE(obj));
    
    JsonObject* jobj = json_node_get_object (obj);
    if (!json_object_has_member(jobj, prop_name))
      throw InvalidMember(prop_name);
    
    return INode( json_object_get_member(jobj, prop_name) );
  };

  GList* keys() {
    if (!is_object())
      throw InvalidType(JSON_NODE_OBJECT, JSON_NODE_TYPE(obj));
    JsonObject* jobj = json_node_get_object (obj);
    return json_object_get_members(jobj);
  };
  
  auto operator [](guint index) {
    if (!is_array())
      throw InvalidType(JSON_NODE_ARRAY, JSON_NODE_TYPE(obj));
    
    JsonArray* jarr = json_node_get_array (obj);
    if (index >= json_array_get_length(jarr))
      throw InvalidIndex(index);
    
    return INode( json_array_get_element(jarr, index) );
  };
  
  static void foreach_callback(JsonArray*, guint, JsonNode* elem, gpointer data) {
    INode node = elem;
    std::function<void(INode&)>* func = reinterpret_cast<std::function<void(INode&)>*>(data);
    (*func)(node);
  }

  void each(std::function<void(INode&)> func) {
    if (!is_array())
      throw InvalidType(JSON_NODE_ARRAY, JSON_NODE_TYPE(obj));
    
    JsonArray* jarr = json_node_get_array (obj);
    gpointer data = &func;
    json_array_foreach_element (jarr, foreach_callback, data);
  }
  
  operator const gchar* () {
    if (!is_value())
      throw InvalidType(JSON_NODE_VALUE, JSON_NODE_TYPE(obj));
    return json_node_get_string(obj);
  };
  
  operator const int () {
    if (!is_value())
      throw InvalidType(JSON_NODE_VALUE, JSON_NODE_TYPE(obj));
    return json_node_get_int(obj);
  };
  
  operator const bool () {
    if (!is_value())
      throw InvalidType(JSON_NODE_VALUE, JSON_NODE_TYPE(obj));
    return json_node_get_boolean(obj);
  };
  
  operator const double () {
    if (!is_value())
      throw InvalidType(JSON_NODE_VALUE, JSON_NODE_TYPE(obj));
    return json_node_get_double(obj);
  };

  void operator = (JsonNode* src) {
    if (src == super::obj)
      return;
    if (!is_null())
      super::decref();
    super::obj = src;
    super::incref();
  };
  
  void operator = (const INode& src) {
    if (src.obj == super::obj)
      return;
    if (!is_null())
      super::decref();
    super::obj = src.obj;
    super::incref();
  };

  void operator = (const gchar* str) {
    json_node_set_string(obj, str);
  }

  void operator = (const double src) {
    json_node_set_double(obj, src);
  }

  void operator = (const int src) {
    json_node_set_int(obj, src);
  }

  void operator = (const bool src) {
    json_node_set_boolean(obj, src);
  }
  
  auto query(const gchar* pattern) {
    GError* error;
    JsonNode* result = json_path_query(pattern, obj, &error);
    if (error)
      throw RuntimeError(error);
    return INode(result);
  }
};

inline INode ref(JsonNode* obj) {
  return INode(obj);
}

}; // namespace Json

#endif
