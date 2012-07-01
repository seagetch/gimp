#ifndef __GLIB_CXX_UTILS_HPP__
#define __GLIB_CXX_UTILS_HPP__

extern "C" {
#include <glib.h>
}

#include "base/scopeguard.hpp"

class MemoryHolder : public ScopeGuard<gchar, void(gpointer)>
{
public:
  MemoryHolder(const gpointer data) : ScopeGuard<gchar, void(gpointer)>((gchar*)data, g_free) {};
  template<typename T> operator T*() { return (T*)obj; };
  template<typename T> operator T* const() { return (T*)obj; };
};

class StringHolder : public ScopeGuard<gchar, void(gpointer)>
{
public:
  StringHolder(gchar* str) : ScopeGuard<gchar, void(gpointer)>(str, g_free) {};
};

template<typename Key, typename Data>
class GHashTableHolder : public ScopeGuard<GHashTable, void(GHashTable*)>
{
public:
  GHashTableHolder(GHashTable* table) : ScopeGuard<GHashTable, void(GHashTable*)>(table, g_hash_table_unref) {};

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
  GListHolder(GList* list) : ScopeGuard<GList, void(GList*)>(list, g_list_free) {};
};

class GObjectRefPtr : public ScopeGuard<GObject, void(gpointer)>
{
public:
  GObjectRefPtr(GObject* object) : ScopeGuard<GObject, void(gpointer)>(object, g_object_unref) {};
};

template<typename Class>
class RefPtr : public GObjectRefPtr
{
public:
  RefPtr(Class* object) : GObjectRefPtr(G_OBJECT(object)) {};
  Class* as(GType type_id) { return G_TYPE_CHECK_INSTANCE_CAST ((obj), type_id, Class); }
};
#endif
