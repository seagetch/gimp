#ifndef __GLIB_CXX_UTILS_HPP__
#define __GLIB_CXX_UTILS_HPP__

extern "C" {
#include <glib.h>
}

#include "base/scopeguard.hpp"

class MemoryHolder : public ScopeGuard<gchar, void(gpointer)>
{
public:
  MemoryHolder()              : ScopeGuard<gchar, void(gpointer)>(NULL        , g_free) {};
  MemoryHolder(gpointer data) : ScopeGuard<gchar, void(gpointer)>((gchar*)data, g_free) {};
  template<typename T> operator T*() { return (T*)obj; };
  template<typename T> operator T* const() { return (T*)obj; };
  MemoryHolder& operator =(gchar* rhs) {
    if (obj != rhs)
      dispose();
    obj = rhs;
    return *this;
  }
};

class StringHolder : public ScopeGuard<gchar, void(gpointer)>
{
public:
  StringHolder() :           ScopeGuard<gchar, void(gpointer)>(NULL, g_free) {};
  StringHolder(gchar* str) : ScopeGuard<gchar, void(gpointer)>(str, g_free) {};
  StringHolder& operator =(gchar* rhs) {
    if (obj != rhs)
      dispose();
    obj = rhs;
    return *this;
  }
};

template<typename Key, typename Data>
class GHashTableHolder : public ScopeGuard<GHashTable, void(GHashTable*)>
{
public:
  GHashTableHolder()                  : ScopeGuard<GHashTable, void(GHashTable*)>(NULL,  g_hash_table_unref) {};
  GHashTableHolder(GHashTable* table) : ScopeGuard<GHashTable, void(GHashTable*)>(table, g_hash_table_unref) {};

  GHashTableHolder& operator =(GHashTable* rhs) {
    if (obj != rhs)
      dispose();
    obj = rhs;
    return *this;
  }
  Data lookup(const Key data) {
    return reinterpret_cast<Data>(g_hash_table_lookup(obj, (const gpointer)data));
  }
  Data operator[](const Key data) {
    return lookup(data);
  }
};

class GListHolder : public ScopeGuard<GList, void(GList*)>
{
public:
  GListHolder()            : ScopeGuard<GList, void(GList*)>(NULL, g_list_free) {};
  GListHolder(GList* list) : ScopeGuard<GList, void(GList*)>(list, g_list_free) {};
  GListHolder& operator =(GList* rhs) {
    if (obj != rhs)
      dispose();
    obj = rhs;
    return *this;
  }
  void append(gpointer data) {
    overwrite( g_list_append((GList*)obj, data) );
  }
  void prepend(gpointer data) {
    overwrite( g_list_prepend((GList*)obj, data) );
  }
  void insert(gpointer data, gint position) {
    overwrite( g_list_insert((GList*)obj, data, position) );
  }
  void remove(gconstpointer data) {
    overwrite( g_list_remove((GList*)obj, data) );
  }
  void remove_link(GList* element) {
    overwrite( g_list_remove_link((GList*)obj, element) );
  }
  void concat(GList* list) {
    overwrite( g_list_concat((GList*)obj, list) );
  }
  void overwrite(GList* list) { obj = list; }
  GList* nth(guint n) { return g_list_nth((GList*)obj, n); }
  gpointer nth_data(guint n) { return g_list_nth_data((GList*)obj, n); }

  void free_full(GDestroyNotify free_func) {
    g_list_free_full((GList*)obj, free_func);
    obj = NULL;
  }
  
  template<typename Item> Item as(guint n) { return reinterpret_cast<Item>(nth_data(n)); }
};

class GObjectRefPtr : public ScopeGuard<GObject, void(gpointer)>
{
public:
  GObjectRefPtr() :                ScopeGuard<GObject, void(gpointer)>(NULL, g_object_unref) {};
  GObjectRefPtr(GObject* object) : ScopeGuard<GObject, void(gpointer)>(object, g_object_unref) {};
  GObjectRefPtr& operator =(GObject* rhs) {
    if (obj != rhs)
      dispose();
    obj = rhs;
    return *this;
  }
};

template<typename Class>
class RefPtr : public GObjectRefPtr
{
public:
  RefPtr() :              GObjectRefPtr() {};
  RefPtr(Class* object) : GObjectRefPtr(G_OBJECT(object)) {};
};


#define DEFINE_REF_PTR(Class, TypeId) \
template<> \
class RefPtr<Class> : public GObjectRefPtr \
{ \
public: \
  operator Class*() { return G_TYPE_CHECK_INSTANCE_CAST ((obj), TypeId, Class); } \
  bool empty() const { return obj == NULL; } \
  bool valid() const { G_TYPE_CHECK_INSTANCE_TYPE((obj), TypeId); } \
  operator bool const() { return !empty() && valid(); } \
  RefPtr& operator =(Class* rhs) { \
    if (obj != G_OBJECT(rhs)) \
      dispose(); \
    obj = G_OBJECT(rhs); \
    return *this; \
  } \
}; \
namespace Ref { typedef RefPtr<Class> Class; };

#endif
