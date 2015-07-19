#ifndef __GLIB_CXX_UTILS_HPP__
#define __GLIB_CXX_UTILS_HPP__

extern "C" {
#include <glib.h>
}

#include "base/scopeguard.hpp"
#include "base/delegators.hpp"

class MemoryHolder : public ScopedPointer<gchar, void(gpointer), g_free>
{
public:
  MemoryHolder(const gpointer data) : ScopedPointer<gchar, void(gpointer), g_free>((gchar*)data) {};
  template<typename T> operator T*() { return (T*)obj; };
  template<typename T> operator T* const() { return (T*)obj; };
};

class StringHolder : public ScopedPointer<gchar, void(gpointer), g_free>
{
public:
  StringHolder(gchar* str) : ScopedPointer<gchar, void(gpointer), g_free>(str) {};
};

template<typename Key, typename Data>
class GHashTableHolder : public ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>
{
public:
  GHashTableHolder(GHashTable* table) : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(table) {};

  Data lookup(const Key data) {
    return reinterpret_cast<Data>(g_hash_table_lookup(obj, (const gpointer)data));
  }
  Data operator[](const Key data) {
    return lookup(data);
  }
};

class GValueWrapper 
{
private:
  GValue value;
  ScopeGuard<GValue*, void(GValue*), g_value_unset> holder;
public:
  GValueWrapper(GValue src) : value(src), holder(&value) {};
  GValueWrapper(gchar val) : holder(&value) {
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_CHAR); 
    g_value_set_schar(&value, val);
  };
  GValueWrapper(bool val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_BOOLEAN); 
    g_value_set_boolean(&value, val);
  };
  GValueWrapper(gint val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_INT); 
    g_value_set_int(&value, val);
  };
  GValueWrapper(guint val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_UINT); 
    g_value_set_uint(&value, val);
  };
  GValueWrapper(glong val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_LONG); 
    g_value_set_long(&value, val);
  };
  GValueWrapper(gulong val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_ULONG); 
    g_value_set_ulong(&value, val);
  };
  GValueWrapper(gfloat val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_FLOAT); 
    g_value_set_float(&value, val);
  };
  GValueWrapper(gdouble val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_DOUBLE); 
    g_value_set_double(&value, val);
  };
  GValueWrapper(GObject* val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_OBJECT); 
    g_value_set_object(&value, val);
  };
  GValueWrapper(gchar* val) : holder(&value) { 
    GValue default_value = G_VALUE_INIT;
    value = default_value;
    g_value_init(&value, G_TYPE_STRING); 
    g_value_set_string(&value, val);
  };
  GValue& ref() { return value; };
  GType type() { return G_VALUE_TYPE(&value); };
  operator const gchar() const { return g_value_get_schar(&value); };
  operator const bool() const { return g_value_get_boolean(&value); };
  operator const gint() const { return g_value_get_int(&value); };
  operator const guint() const { return g_value_get_uint(&value); };
  operator const glong() const { return g_value_get_long(&value); };
  operator const gulong() const { return g_value_get_ulong(&value); };
  operator const gfloat() const { return g_value_get_float(&value); };
  operator const gdouble() const { return g_value_get_double(&value); };
  operator const gchar*() const { return g_value_get_string(&value); };
  operator const GObject*() const { return G_OBJECT(g_value_get_object(&value)); };
};

class GListHolder : public ScopedPointer<GList, void(GList*), g_list_free>
{
public:
  GListHolder(GList* list) : ScopedPointer<GList, void(GList*), g_list_free>(list) {};
};

template<class T>
class GObjectHolder : public ScopedPointer<T, void(gpointer), g_object_unref>
{
  typedef ScopedPointer<T, void(gpointer), g_object_unref> super;
public:
  GObjectHolder() : super(NULL) {};
  GObjectHolder(T* object) : super(object) {};

  GObject* as_object() { return G_OBJECT(super::obj); }
  void incref() { g_object_ref(as_object()); };
  void decref() { g_object_unref(as_object()); };
  void freeze() { g_object_freeze_notify(as_object()); };
  void thaw()   { g_object_thaw_notify(as_object()); };

  struct InvalidIndex { const gchar* name; InvalidIndex(const gchar* n) : name(n) {;}; };
  GValueWrapper get(const gchar* prop_name) {
    GObject* object = as_object();
    GObjectClass* class_obj = G_OBJECT_GET_CLASS(object);
    GParamSpec *pspec = g_object_class_find_property (class_obj, prop_name);
    if (!pspec) throw InvalidIndex(prop_name);
    GValue result = G_VALUE_INIT;
    g_print("brush:value:init=%s\n", g_type_name(pspec->value_type));
    g_value_init(&result, pspec->value_type);
    g_object_get_property(object, prop_name, &result);
    g_print("brush::name=%s\n", g_value_get_string(&result));
    return result;
  };

  void set(const gchar* prop_name, GValueWrapper value) {
    GObject* object = as_object();
    g_object_set_property(object, prop_name, &value.ref());
  };

  template<typename Function>
  Delegator::Connection* connect(const gchar* event, 
            Delegator::Delegator<Function>* delegator, 
            bool after=false) 
  {
      return g_signal_connect_delegator (as_object(), event, delegator, after);
  };

  void operator = (T* src) {
    if (!is_null(super::obj))
      super::decref();
    super::obj = src;
  };

};

template<class T>
class GWrapper : public GObjectHolder<T>
{
  typedef GObjectHolder<T> super;
public:
  GWrapper() : super(NULL) { };
  GWrapper(T* object) : super(object) { 
    super::incref();
  };

  operator GObject* () { return super::as_object(); };
  operator T* () { return super::ptr(); };
  T* operator ->() { return super::ptr(); };

  void operator = (T* src) {
    if (!is_null(super::obj))
      super::decref();
    super::obj = src;
    super::incref();
  };

};
#endif
