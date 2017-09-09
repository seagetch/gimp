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

namespace JSON {

using namespace GLib;
using namespace Delegators;


class Node : public ScopedPointer<JsonNode, void(JsonNode*), json_node_unref>
{
  typedef ScopedPointer<JsonNode, void(JsonNode*), json_node_unref> super;
public:
  Node() : super(NULL) {};
  Node(JsonNode* node) : super(node) {};
  Node(Node&& src) : super(src.obj) { src.obj = NULL; }

  void incref() { if (obj) json_node_ref(obj); };
  void decref() { if (obj) json_node_unref(obj); };

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
  
  bool is_object() const { return obj && JSON_NODE_HOLDS_OBJECT(obj); }
  bool is_array()  const { return obj && JSON_NODE_HOLDS_ARRAY (obj); }
  bool is_value()  const { return obj && JSON_NODE_HOLDS_VALUE (obj); }
  bool is_null()   const { return !obj || json_node_is_null (obj); }

  struct InvalidMember { const gchar* name; InvalidMember(const gchar* n) : name(n) {;}; };
  struct InvalidIndex  { const int index; InvalidIndex(const int i) : index(i) {;}; };
  struct InvalidType { const JsonNodeType specified; const JsonNodeType actual; InvalidType(const JsonNodeType s, const JsonNodeType a) : specified(s), actual(a) {;}; };
  struct RuntimeError  { const GError* error; RuntimeError(const GError* e) : error(e) {;}; };
  
  auto operator[](const gchar* prop_name) {
    if (!is_object())
      return INode();
    
    JsonObject* jobj = json_node_get_object (obj);
    return INode( json_object_get_member(jobj, prop_name) );
  };

  GList* keys() {
    if (!is_object())
      throw InvalidType(JSON_NODE_OBJECT, JSON_NODE_TYPE(obj));
    JsonObject* jobj = json_node_get_object (obj);
    return json_object_get_members(jobj);
  };
  
  bool has(const gchar* prop_name) {
    if (!is_object())
      return false;
    JsonObject* jobj = json_node_get_object (obj);
    return json_object_has_member (jobj, prop_name);
  }

  gint length() {
    if (!is_array())
      return -1;
    JsonArray* jarr = json_node_get_array (obj);
    return json_array_get_length(jarr);
  }

  auto operator [](guint index) {
    if (!is_array())
      return INode();
    
    JsonArray* jarr = json_node_get_array (obj);
    if (index >= json_array_get_length(jarr))
      throw InvalidIndex(index);
    
    return INode( json_array_get_element(jarr, index) );
  };
  
  template<typename T>
  bool copy_values(T** _data, gsize* _length) {
    Nullable<gsize> length (_length);
    Nullable<T*> data   (_data);
    length = this->length();
    data   = g_new0(T, length);
    int i = 0;
    each([&](auto it){
      Nullable<T> value(data[i++]);
      value = it;
    });
  }

  static void foreach_callback(JsonArray*, guint, JsonNode* elem, gpointer data) {
    INode node = elem;
    std::function<void(INode&)>* func = reinterpret_cast<std::function<void(INode&)>*>(data);
    (*func)(node);
  }

  void each(std::function<void(INode&)> func) {
    if (!is_array())
      return;
    
    JsonArray* jarr = json_node_get_array (obj);
    gpointer data = &func;
    json_array_foreach_element (jarr, foreach_callback, data);
  }
  
  operator JsonNode*() { return ptr(); };

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
  
  operator const gint16 () {
    return (int)(*this);
  };

  operator const guint16 () {
    return (int)(*this);
  };

  operator const gint8 () {
    return (int)(*this);
  };

  operator const guint8 () {
    return (int)(*this);
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
    if (obj)
      json_node_set_string(obj, str);
  }

  void operator = (const double src) {
    if (obj)
      json_node_set_double(obj, src);
  }

  void operator = (const int src) {
    if (obj)
      json_node_set_int(obj, src);
  }

  void operator = (const bool src) {
    if (obj)
      json_node_set_boolean(obj, src);
  }
  
  GType value_type() const {
    if (obj)
      return json_node_get_node_type(obj);
    return G_TYPE_NONE;
  }

  auto query(const gchar* pattern) {
    if (!obj)
      return INode();
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


class Builder : public ScopedPointer<JsonBuilder, void(gpointer), g_object_unref> {
  using super = ScopedPointer<JsonBuilder, void(gpointer), g_object_unref>;
public:
  Builder() : super(json_builder_new()) { };
  Builder(JsonBuilder* builder) : super(builder) { };
};


class IBuilder : public Builder {
public:
  struct null { };

  template<typename F>
  class LazyObjectEvaluator {
    F func;
    IBuilder* builder;
  public:
    LazyObjectEvaluator(F f, IBuilder* b) : func(f), builder(b)  {};
    IBuilder& eval() const {
      json_builder_begin_object(builder->obj);
      func(*builder);
      json_builder_end_object(builder->obj);
      return *builder;
    }
  };

  template<typename F>
  class LazyArrayEvaluator {
    F func;
    IBuilder* builder;
  public:
    LazyArrayEvaluator(F f, IBuilder* b) : func(f), builder(b)  {};
    IBuilder& eval() const {
      json_builder_begin_array(builder->obj);
      func(*builder);
      json_builder_end_array(builder->obj);
      return *builder;
    }
  };

  IBuilder(const Builder& b) : Builder(b) { g_object_ref(G_OBJECT(obj)); };
  IBuilder(const IBuilder& b) : Builder(b.obj) { g_object_ref(G_OBJECT(obj)); };

  JsonNode* get_root() {
    return json_builder_get_root(obj);
  }

  template<typename F>
  auto object(F f) {
    return LazyObjectEvaluator<F>(f, this);
  }

  template<typename F>
  auto array(F f) {
    return LazyArrayEvaluator<F>(f, this);
  }

  template<typename F>
  void with_object(F f) {
    *this = object(f);
  }

  template<typename F>
  void with_array(F f) {
    *this = array(f);
  }

  IBuilder& operator[](const gchar* name) {
    json_builder_set_member_name(obj, name);
    return *this;
  }

  template<typename F>
  IBuilder& operator = (const LazyObjectEvaluator<F>& src) {
    src.eval();
    return *this;
  }

  template<typename F>
  IBuilder& operator = (const LazyArrayEvaluator<F>& src) {
    src.eval();
    return *this;
  }

  IBuilder& operator = (int value) {
    json_builder_add_int_value(obj, value);
    return *this;
  }

  IBuilder& operator = (const gchar* value) {
    json_builder_add_string_value(obj, value);
    return *this;
  }

  IBuilder& operator = (const double value) {
    json_builder_add_double_value(obj, value);
    return *this;
  }

  IBuilder& operator = (const bool value) {
    json_builder_add_boolean_value(obj, value);
    return *this;
  }

  IBuilder& operator = (const null value) {
    json_builder_add_null_value(obj);
    return *this;
  }

  IBuilder& operator = (JsonNode* value) {
    json_builder_add_value(obj, value);
    return *this;
  }

  IBuilder& operator = (Node& value) {
    json_builder_add_value(obj, value);
    return *this;
  }

  template<typename Arg, typename... Args>
  IBuilder& operator()(Arg a, Args... args) {
    (*this) = a;
    return (*this)(args...);
  };

  IBuilder& operator()() { return *this; };

  template<typename... Args>
  auto array_with_values(Args... args) {
    return array([&](auto it){
      it(args...);
    });
  }

  template<typename T>
  auto array_from(T* data, gsize length) {
    return array([data, length](auto it){
      for (int i = 0; i < length; i ++)
        it = data[i];
    });
  }

};

inline auto ref(const Builder& builder) {
  return IBuilder(builder);
}

template<typename F>
inline auto build(F f) {
  Builder builder;
  IBuilder ibuilder = ref(builder);
  f(ibuilder);
  return ibuilder.get_root();
}

template<typename F>
inline auto build_object(F f) {
  return build([&f](auto it) {
    it.with_object(f);
  });
}

template<typename F>
inline auto build_array(F f) {
  return build([&f](auto it) {
    it.with_array(f);
  });
}

}; // namespace JSON

#endif
