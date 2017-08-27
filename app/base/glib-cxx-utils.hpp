#ifndef __GLIB_CXX_UTILS_HPP__
#define __GLIB_CXX_UTILS_HPP__

extern "C" {
#include <glib.h>
#include <glib-object.h>
#include "core/core-types.h"
#include "core/gimpparamspecs.h"
}
#include <iterator>

#include "base/scopeguard.hpp"
#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"

using namespace Delegators;

namespace GLib {


class MemoryHolder : public ScopedPointer<gchar, void(gpointer), g_free>
{
public:
  MemoryHolder(const gpointer data) : ScopedPointer<gchar, void(gpointer), g_free>((gchar*)data) {};
  template<typename T> operator T*() { return (T*)obj; };
  template<typename T> operator T* const() { return (T*)obj; };
};


class CString : public ScopedPointer<gchar, void(gpointer), g_free>
{
public:
  CString() : ScopedPointer<gchar, void(gpointer), g_free>() {};
  CString(gchar* str) : ScopedPointer<gchar, void(gpointer), g_free>(str) {};
  CString(CString&& src) : ScopedPointer<gchar, void(gpointer), g_free>(src.obj) {
    src.obj = NULL;
  }
  CString(const CString& src) : ScopedPointer<gchar, void(gpointer), g_free>(g_strdup(src.obj)) { }
  CString& operator =(gchar* str) {
    if (obj == str)
      return *this;
    if (obj)
      g_free(obj);
    obj = str;
    return *this;
  };
};


class StringList : public ScopedPointer<gchar*, void(gchar**), g_strfreev>
{
  using super = ScopedPointer<gchar*, void(gchar**), g_strfreev>;
public:
  StringList() : super(NULL) {};
  StringList(gchar** list) : super(list) {};
  StringList(StringList&& src) : super(src.obj) { src.obj = NULL; }
  gchar** begin() { return obj; }
  gchar** end() {
    for (gchar** str = obj; true; str++)
      if (!*str) return str;
  }
  gchar* const* begin() const { return obj; }
  gchar* const* end() const {
    for (gchar** str = obj; true; str++)
      if (!*str) return str;
  }
  operator gchar**() { return obj; }
  operator gchar* const *() const { return obj; }
  StringList& operator = (gchar** str) {
    if (obj)
      g_strfreev(obj);
    obj = str;
  }
};


template<typename T>
void g_string_free_all(T* str) { g_string_free((GString*)str, TRUE); }
class String : public ScopedPointer<GString, void(GString*), g_string_free_all<GString> >
{
public:
  String(GString* str) : ScopedPointer<GString, void(GString*), g_string_free_all<GString> >(str) {};
  const gchar* str()    const { return obj->str; };
  const gint   length() const { return obj->len; };
  gchar* str()    { return obj->str; };
  gint   length() { return obj->len; };

  operator gchar*() { return str(); }

  String& operator += (const gchar* val) {
    g_string_append(obj, val);
    return *this;
  };

  String& operator += (const gchar val) {
    g_string_append_c(obj, val);
    return *this;
  };

};


class List : public ScopedPointer<GList, void(GList*), g_list_free>
{
public:
  List(GList* list) : ScopedPointer<GList, void(GList*), g_list_free>(list) {};
  List(List&& src) : ScopedPointer<GList, void(GList*), g_list_free>(src.obj) {
    src.obj = NULL;
  };
  List(const List& src) : ScopedPointer<GList, void(GList*), g_list_free>(g_list_copy(src.obj)) { }
};


inline auto hold(GList* list) { return static_cast<List&&>(List(list)); }


template<class T>
class IList {
  GList* obj;
  template<typename F>
  static void foreach_callback(gpointer data, gpointer user) {
    F* func = reinterpret_cast<F*>(user);
    (*func)(T(data));
  };
public:
  IList(GList* list) : obj(list) { };
  IList(IList&& src) : obj(src.obj) { src.obj = NULL; };
  IList(const IList& src) : obj(src.obj) { };
  T operator[](int index) {
    return T(g_list_nth_data(obj, index));
  }
  template<typename F>
  void each(F each_func) {
    g_list_foreach (obj, foreach_callback<F>, &each_func);
  }
  class Iterator {
    GList* iter;
  public:
    Iterator() : iter(NULL) { }
    Iterator(GList* list) : iter(list) { }
    Iterator(Iterator&& src) : iter(src.iter) { src.iter = NULL; };
    Iterator(const Iterator& src) : iter(src.iter) { };
    bool operator == (const Iterator& left) { return iter == left.iter; }
    bool operator != (const Iterator& left) { return !(*this == left); }
    Iterator& operator ++() { iter = g_list_next(iter); return *this; }
    operator T() { return reinterpret_cast<T>(iter->data); }
    auto operator *() { return T(*this); }
  };
  Iterator begin() const { return Iterator(obj); }
  Iterator end() const { return Iterator(); }
};


template<typename Data>
inline auto ref(GList* list) { return static_cast<IList<Data>&&>(IList<Data>(list)); }

}; // namespace GLib


namespace std {
template<typename T> auto begin(const GLib::IList<T>& list) { return list.begin(); }
template<typename T> auto end(const GLib::IList<T>& list) { return list.end(); }
};


namespace GLib {

class Array : public ScopedPointer<GArray, void(GArray*), g_array_unref>
{
protected:
  void incref() { g_array_ref(obj); }
  void decref() { g_array_unref(obj); }
public:
  Array() : ScopedPointer<GArray, void(GArray*), g_array_unref>(NULL) {};
  Array(GArray* arr) : ScopedPointer<GArray, void(GArray*), g_array_unref>(arr) {};
  Array(Array&& src) : ScopedPointer<GArray, void(GArray*), g_array_unref>(src.obj) {
    src.obj = NULL;
  };
  Array(const Array& src) : ScopedPointer<GArray, void(GArray*), g_array_unref>(src.obj) { incref(); }

  Array& operator =(GArray*&& arr) {
    obj = arr;
    arr = NULL;
    return *this;
  }

  Array& operator =(Array&& src) {
    obj = src.obj;
    src.obj = NULL;
    return *this;
  }
};


inline auto hold(GArray* arr) { return static_cast<Array&&>(Array(arr)); }


template<typename Data>
class IArray : public Array
{
public:
  IArray(GArray* arr) : Array(arr) { incref(); };
  IArray(const IArray& src) : Array(src.obj) { incref(); }
  Data& operator[](int index) {
    Data* array = (Data*)(obj->data);
    return array[index];
  }
  int size() { return obj->len; }
  void append(Data data) { g_array_append_val(obj, data); };
  void prepend(Data data) { g_array_prepend_val(obj, data); };
  void insert(int index, Data data) { g_array_insert_val(obj, index, data); };
  void remove(int index) { g_array_remove_index(obj, index); };
  Data* begin() { return &(*this)[0]; }
  Data* end()   { return &(*this)[size()]; }
};


template<typename Data>
inline auto ref(GArray* arr) { return static_cast<IArray<Data>&&>(IArray<Data>(arr)); }

template<typename Data>
inline auto ref(GLib::Array& arr) { return static_cast<IArray<Data>&&>(IArray<Data>(arr.ptr())); }

}; // namespace GLib


namespace std {
template<typename T> auto begin(const GLib::IArray<T>& arr) { return arr.begin(); }
template<typename T> auto end  (const GLib::IArray<T>& arr) { return arr.end(); }
}; // namespace std


namespace GLib
{

class HashTable : public ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>
{
public:
  HashTable() : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(NULL) {};
  HashTable(GHashTable* table) : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(table) {};
  HashTable(HashTable&& src) : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(src.obj) { src.obj = NULL; }
  HashTable(const HashTable& src) : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(src.obj) { incref(); }
  void incref() {
    if (obj)
      g_hash_table_ref(obj);
  }
  void decref() {
    if (obj)
      g_hash_table_unref(obj);
  }
  HashTable& operator = (GHashTable* table) {
    if (obj == table)
      return *this;
    decref();
    obj = table;
    return *this;
  }
};


inline auto hold(GHashTable* hashtable) { return static_cast<HashTable&&>(HashTable(hashtable)); }


template<typename Key, typename Data>
class IHashTable : public HashTable
{
  template<typename F>
  static void foreach_callback(gpointer key, gpointer data, gpointer user) {
    F* func = reinterpret_cast<F*>(user);
    (*func)((const Key)key, (Data)data);
  };
public:
  IHashTable(GHashTable* table) : HashTable(table) { incref(); };
  IHashTable(const IHashTable& src)   : HashTable(src.obj) { incref(); };

  const Data lookup(const Key key) const {
    return (Data)(g_hash_table_lookup(obj, (gconstpointer) key));
  }
  const Data operator[](const Key key) const {
    return lookup(key);
  }
  bool insert(const Key key, Data value) {
    g_hash_table_insert(ptr(), (gpointer)key, (gpointer)value);
  }
  template<typename F>
  void each(F each_func) {
    g_hash_table_foreach (obj, foreach_callback<F>, &each_func);
  }
  IHashTable& operator = (GHashTable* table) {
    if (obj == table)
      return *this;
    decref();
    obj = table;
    incref();
    return *this;
  }
};


template<typename Key, typename Data>
inline auto ref(GHashTable* hashtable) { return static_cast<IHashTable<Key, Data>&&>(IHashTable<Key, Data>(hashtable)); }


class synchronized : public ScopeGuard<GMutex*, void(GMutex*), g_mutex_unlock, is_null<GMutex*> >
{
  using super = ScopeGuard<GMutex*, void(GMutex*), g_mutex_unlock, is_null<GMutex*> >;
  synchronized(const synchronized& src) = delete;
public:
  synchronized(GMutex* mutex) : super(mutex) { g_mutex_lock(mutex); }

  template<typename F>
  static void with_synchronized(GMutex* mutex, F f) {
    synchronized locker(mutex);
    f(&locker);
  }

  void exit() { if (obj) g_mutex_unlock(obj); obj = NULL; }

  template<typename F>
  void unlocked(F f) {
    if (!obj) return;
    g_mutex_unlock(obj);
    f(this);
    g_mutex_lock(obj);
  }
};

class Regex : public ScopedPointer<GRegex, void(GRegex*), g_regex_unref>
{
public:


  class Matched : public ScopedPointer<GMatchInfo, void(GMatchInfo*), g_match_info_unref>
  {
    bool matched;

  public:
    Matched(GMatchInfo* info) : ScopedPointer<GMatchInfo, void(GMatchInfo*), g_match_info_unref>(info), matched(true) {};
    Matched(Matched& src) : ScopedPointer<GMatchInfo, void(GMatchInfo*), g_match_info_unref>(src.obj), matched(true) { g_match_info_ref(obj); };
    gchar* operator [](int index) const { return g_match_info_fetch(obj, index); };
    operator bool() {
      return obj && matched && (matched = g_match_info_matches(obj));
    };
    int count() const { return g_match_info_get_match_count(obj); }
    Matched& operator ++(int) {
      matched = matched && g_match_info_next(obj, NULL);
      return *this;
    }
  };

  Regex(GRegex* regex) : ScopedPointer<GRegex, void(GRegex*), g_regex_unref>(regex) {};

  Regex(const gchar* pattern) : ScopedPointer<GRegex, void(GRegex*), g_regex_unref>(
      g_regex_new(pattern, (GRegexCompileFlags)G_REGEX_OPTIMIZE, (GRegexMatchFlags)0, NULL )) {};

  Matched match(gchar* str) {
    GMatchInfo* info;
    g_regex_match(obj, str, (GRegexMatchFlags)0, &info);
    return Matched(info);
  };

  template<typename F>
  bool match(gchar* str, F matched_callback) {
    GMatchInfo* info;
    g_regex_match(obj, str, (GRegexMatchFlags)0, &info);
    Matched matched(info);
    bool result = false;
    while (matched) {
      result = true;
      matched_callback(matched);
      matched ++;
    }
    return result;
  };

  gchar* replace(const gchar* str, const gchar* replace_str) {
    return g_regex_replace(obj, str, -1, 0, replace_str, GRegexMatchFlags(0), NULL);
  }

};

class regex_case {
  CString str;
  bool matched;

public:
  regex_case(const gchar* s) : str(g_strdup(s)), matched(false) { };
  regex_case& when(Regex& regex, std::function<void(Regex::Matched&)> callback) {
    if (!matched)
      matched = (regex.match(str, callback));
    return *this;
  };

  regex_case& when(const gchar* pattern, std::function<void(Regex::Matched&)> callback) {
    auto regex = Regex(pattern);
    return when(regex, callback);
  };

  void otherwise(std::function<void()> callback) {
    if (!matched)
      callback();
  };
};


template<typename T> inline const T get_value(const GValue& src) {
  T::__this_should_generate_error_on_compilation;
  g_error("Fallback to get_value for %s\n", typeid(T).name());
  return T();
}

template<typename T> inline void set_value(GValue& src, T value) {
  T::__this_should_generate_error_on_compilation;
  g_error("Fallback to set_value for %s\n", typeid(T).name());
}

//#define G_TYPE_INTERFACE
template<> inline GType g_type<gchar>() { return G_TYPE_CHAR; }
template<> inline const gchar get_value<gchar>(const GValue& src) { return g_value_get_schar(&src); }
template<> inline void set_value<gchar>(GValue& src, gchar value) { g_value_set_schar(&src, value); }

template<> inline GType g_type<guchar>() { return G_TYPE_UCHAR; }
template<> inline const guchar get_value<guchar>(const GValue& src) { return g_value_get_uchar(&src); }
template<> inline void set_value<guchar>(GValue& src, guchar value) { g_value_set_uchar(&src, value); }

template<> inline GType g_type<bool>() { return G_TYPE_BOOLEAN; }
template<> inline const bool get_value<bool>(const GValue& src) { return g_value_get_boolean(&src); }
template<> inline void set_value<bool>(GValue& src, bool value) { g_value_set_boolean(&src, value); }

template<> inline GType g_type<gint32>() { return G_TYPE_INT; }
template<> inline const gint32 get_value<gint32>(const GValue& src) { return g_value_get_int(&src); }
template<> inline void set_value<gint32>(GValue& src, gint32 value) { g_value_set_int(&src, value); }

template<> inline GType g_type<guint32>() { return G_TYPE_UINT; }
template<> inline const guint32 get_value<guint32>(const GValue& src) { return g_value_get_uint(&src); }
template<> inline void set_value<guint32>(GValue& src, guint32 value) { g_value_set_uint(&src, value); }

template<> inline GType g_type<glong>() { return G_TYPE_LONG; }
template<> inline const glong get_value<glong>(const GValue& src) { return g_value_get_long(&src); }
template<> inline void set_value<glong>(GValue& src, glong value) { g_value_set_long(&src, value); }

template<> inline GType g_type<gulong>() { return G_TYPE_ULONG; }
template<> inline const gulong get_value<gulong>(const GValue& src) { return g_value_get_ulong(&src); }
template<> inline void set_value<gulong>(GValue& src, gulong value) { g_value_set_ulong(&src, value); }

template<> inline GType g_type<gfloat>() { return G_TYPE_FLOAT; }
template<> inline const gfloat get_value<gfloat>(const GValue& src) { return g_value_get_float(&src); }
template<> inline void set_value<gfloat>(GValue& src, gfloat value) { g_value_set_float(&src, value); }

template<> inline GType g_type<gdouble>() { return G_TYPE_DOUBLE; }
template<> inline const gdouble get_value<gdouble>(const GValue& src) { return g_value_get_double(&src); }
template<> inline void set_value<gdouble>(GValue& src, gdouble value) { g_value_set_double(&src, value); }

template<> inline GType g_type<gchar*>() { return G_TYPE_STRING; }
template<> inline gchar* const get_value<gchar*>(const GValue& src) { return (gchar* const)g_value_get_string(&src); }
template<> inline void set_value<gchar*>(GValue& src, gchar* value) { g_value_set_string(&src, value); }

template<> inline GType g_type<gpointer>() { return G_TYPE_POINTER; }
template<> inline const gpointer get_value<gpointer>(const GValue& src) { return g_value_get_pointer(&src); }
template<> inline void set_value<gpointer>(GValue& src, gpointer value) { g_value_set_pointer(&src, value); }

template<> inline GType g_type<GObject*>() { return G_TYPE_OBJECT; }
template<> inline GObject* const get_value<GObject*>(const GValue& src) { return (GObject* const)G_OBJECT(g_value_get_object(&src)); }
template<> inline void set_value<GObject*>(GValue& src, GObject* value) { g_value_set_object(&src, value); }


//#define G_TYPE_INT64
//#define G_TYPE_UINT64
//#define G_TYPE_ENUM
//#define G_TYPE_FLAGS


template<typename _Instance>
class FundamentalTraits {

public:
  typedef _Instance Instance;

  static GType get_type() { return g_type<Instance>(); };

  static GValue _default() {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, get_type());
    return value;
  };

  static const Instance cast(const GValue& src) {
    GValue value = _default();
    g_value_transform(&src, &value);
    return get_value<Instance>(value);
  };

  static GValue init(Instance src) {
    GValue value = _default();
    set_value<Instance>(value, src);
    return value;
  }
};


template<bool IsOwner>
inline void g_value_finalize(GValue* value) {
  if (value) {
    g_value_unset(value);
    if (IsOwner)
      g_free(value);
  }
};


template<bool IsOwner>
class ValueRef : public ScopedPointer<GValue, void(GValue*), g_value_finalize<IsOwner>>
{
public:
  ValueRef(GValue* src) : ScopedPointer<GValue, void(GValue*), g_value_finalize<IsOwner> >(src) { };
  ValueRef(ValueRef&& src) : ScopedPointer<GValue, void(GValue*), g_value_finalize<IsOwner> >(src.obj) {
    src.obj = NULL;
  };
  GType type() { return G_VALUE_TYPE(this->obj); };

  template<typename T>
  operator const T() const {
    return FundamentalTraits<T>::cast(*this->obj);
  };

  operator const gchar*() const {
    return FundamentalTraits<gchar*>::cast(*this->obj);
  };

  operator const GObject*() const {
    return FundamentalTraits<GObject*>::cast(*this->obj);
  };

};


inline ValueRef<false> ref(GValue* src) { return ValueRef<false>(src); }


class Value : public ValueRef<true>
{
public:

  Value(const GValue& src) : ValueRef<true>(g_new0(GValue, 1)) {
    GValue default_value = G_VALUE_INIT;
    *obj = default_value;
    g_value_init(obj, G_VALUE_TYPE(&src));
    g_value_copy(&src, obj);
  };

  Value(const GValue* src) : ValueRef<true>(g_new0(GValue, 1)) {
    GValue default_value = G_VALUE_INIT;
    *obj = default_value;
    g_value_init(obj, G_VALUE_TYPE(src));
    g_value_copy(src, obj);
  };

  Value(GValue&& src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = src;
    GValue default_value = G_VALUE_INIT;
    src = default_value;
  };

  Value(const Value& src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = *src.obj;
  };

  Value(Value&& src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = *src.obj;
    GValue default_value = G_VALUE_INIT;
    *src.obj = default_value;
  };

  Value(const gchar src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gchar>::init(src);
  };

  Value(const guchar src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<guchar>::init(src);
  };

  Value(const bool src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<bool>::init(src);
  };

  Value(const gint32 src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gint32>::init(src);
  };

  Value(const guint32 src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<guint32>::init(src);
  };

  Value(const glong src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<glong>::init(src);
  };

  Value(const gulong src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gulong>::init(src);
  };

  Value(const gfloat src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gfloat>::init(src);
  };

  Value(const gdouble src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gdouble>::init(src);
  };

  Value(const gpointer src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gpointer>::init(src);
  };

  Value(const gchar* src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<gchar*>::init(g_strdup(src));
  };

  Value(GObject* src) : ValueRef<true>(g_new0(GValue, 1)) {
    *obj = FundamentalTraits<GObject*>::init(src);
  };

  GValue& ref() { return *obj; };
};


inline auto  move(GValue&& src) { return static_cast<Value&&>(Value(static_cast<GValue&&>(src))); }

inline Value dup(const GValue& src) { return Value(src); }


template<class T>
class Object : public ScopedPointer<T, void(gpointer), g_object_unref>
{
  typedef ScopedPointer<T, void(gpointer), g_object_unref> super;

public:
  Object() : super(NULL) {};
  Object(T* object) : super(object) {};
  Object(Object&& src) : super(src.obj) { src.obj = NULL; }

  GObject* as_object() { return G_OBJECT(super::obj); }
  const GObject* as_object() const { return G_OBJECT(super::obj); }

  void incref() { g_object_ref(as_object()); };
  void decref() { g_object_unref(as_object()); };

  void operator = (T* src) {
    if (super::obj == src)
      return;
    if (!is_null(super::obj))
      decref();
    super::obj = src;
  };

};


template<class T>
inline Object<T>&& hold(T* src) { return static_cast<Object<T>&&>(Object<T>(src)); }


template<typename T>
class IObject : public Object<T>
{
  typedef Object<T> super;

public:

  IObject() : super(NULL) { };

  IObject(T* object) : super(object) {
    super::incref();
  };

  IObject(const IObject& src) : super (src.obj) {
    super::incref();
  }

  void freeze() { g_object_freeze_notify(super::as_object()); };
  void thaw()   { g_object_thaw_notify(super::as_object()); };

  struct InvalidIndex { const gchar* name; InvalidIndex(const gchar* n) : name(n) {;}; };
  
  auto get(const gchar* prop_name) {
    GObject* object = super::as_object();
    GObjectClass* class_obj = G_OBJECT_GET_CLASS(object);
    GParamSpec *pspec = g_object_class_find_property (class_obj, prop_name);

    if (!pspec) throw InvalidIndex(prop_name);

    GValue result = G_VALUE_INIT;
    g_value_init(&result, pspec->value_type);
    g_object_get_property(object, prop_name, &result);

    return move(static_cast<GValue&&>(result));
  };

  void set(const gchar* prop_name, Value value) {
    GObject* object = super::as_object();
    g_object_set_property(object, prop_name, &value.ref());
  };

  template<typename Ret, typename... Args>
  Delegators::Connection* connect(
      const gchar* event,
      Delegators::Delegator<Ret, Args...>* delegator,
      bool after=false) {

    return g_signal_connect_delegator (super::as_object(), event, delegator, after);

  };

  operator GObject* () { return super::as_object(); };
  operator T*       () { return super::ptr(); };
  operator gpointer () { return super::ptr(); }
  template<typename G>
  operator G*       () { return GLib::Traits<G>::cast(super::obj); };
  template<typename G>
  operator const G* () { return GLib::Traits<G>::cast(super::obj); };
  T* operator ->    () { return super::ptr(); };
  operator bool     () { return super::ptr(); };

  void operator = (T* src) {
    if (src == super::obj)
      return;
    if (!is_null(super::obj))
      super::decref();
    super::obj = src;
    super::incref();
  };
  
  void operator = (const IObject& src) {
    if (src.obj == super::obj)
      return;
    if (!is_null(super::obj))
      super::decref();
    super::obj = src.obj;
    super::incref();
  };

  template<typename Ret, typename C, typename... Args>
  class BoundMethod {
  protected:
    typedef Ret Method (C*o, Args...);
    Method * method;
    C* obj;
  public:
    BoundMethod(C* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) {
      if (obj)
        return (*method)(obj, args...);
      else
        return Ret();
    };
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename Ret, typename C, typename... Args>
  class ConstBoundMethod {
  protected:
    typedef Ret Method (const C*o, Args...);
    Method * method;
    const C* obj;
  public:
    ConstBoundMethod(const C* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) {
      if (obj)
        return (*method)(obj, args...);
      else
        return Ret();
    };
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename Ret, typename C, typename... Args>
  BoundMethod<Ret, C, Args...> operator []( Ret f(C*, Args...) ) {
    return BoundMethod<Ret, C, Args...>(GLib::Traits<C>::cast(super::obj), f);
  };

  template<typename Ret, typename C, typename... Args>
  ConstBoundMethod<Ret, C, Args...> operator []( Ret f(const C*, Args...) ) {
    return ConstBoundMethod<Ret, C, Args...>(GLib::Traits<C>::cast(super::obj), f);
  };

  template<typename Ret, typename... Args>
  BoundMethod<Ret, T, Args...> operator []( Ret f(T*, Args...) ) {
    return BoundMethod<Ret, T, Args...>(super::obj, f);
  };

  template<typename Ret, typename... Args>
  ConstBoundMethod<Ret, T, Args...> operator []( Ret f(const T*, Args...) ) {
    return ConstBoundMethod<Ret, T, Args...>(super::obj, f);
  };

  template<typename Ret, typename... Args>
  BoundMethod<Ret, void, Args...> operator []( Ret f(gpointer, Args...) ) {
    return BoundMethod<Ret, void, Args...>(super::obj, f);
  };

  template<typename Ret, typename... Args>
  ConstBoundMethod<Ret, void, Args...> operator []( Ret f(gconstpointer, Args...) ) {
    return ConstBoundMethod<Ret, void, Args...>(super::obj, f);
  };

  class GValueAssigner {
    IObject* owner;
    const gchar* name;
  public:
    GValueAssigner(IObject* o, const gchar* n) : owner(o), name(n) { };
    void operator =(const Value&& value) { owner->set(name, value); }
  };

  //  GValueAssigner operator [](const gchar* name) { return GValueAssigner(this); }
  Value operator[] (const gchar* name) { return this->get(name); }
  template<typename D>
  Delegators::Connection* connect(const gchar* signal_name, D d) {
    return g_signal_connect_delegator(G_OBJECT(super::obj), signal_name, d);
  }
};


template<class T>
inline IObject<T> ref(T* obj) {
  return IObject<T>(obj);
}


template<typename W, typename T>
class DelegatorProxy {

protected:
  W* widget;
  T* obj;

public:

  DelegatorProxy(W* w, T* d) : widget(w), obj(d) { };
  virtual ~DelegatorProxy() {};

  template<typename F>
  DelegatorProxy connect(const gchar* name, F f) {
    g_signal_connect_delegator (G_OBJECT(widget), name, Delegators::delegator(obj, f));
    return *this;
  }
  ;
  template<typename F>
  Delegators::Connection* connecth(const gchar* name, F f) {
    return g_signal_connect_delegator (G_OBJECT(widget), name, Delegators::delegator(obj, f));
  };
};


template<typename W, typename T>
class Decorator : public DelegatorProxy<W, T> {

public:

  Decorator(W* w, T* d) : DelegatorProxy<W, T>(w, d) {
    CString attr = g_strdup_printf("_decorator%lx", (gulong)this->obj);
    g_object_set_cxx_object(G_OBJECT(this->widget), attr.ptr(), this);
  };

  virtual ~Decorator() {
    if (this->obj) {
      delete this->obj;
      this->obj = NULL;
    }
  }

};

template<typename W, typename T>
DelegatorProxy<W, T>* delegator(W* w, T* d) { return new DelegatorProxy<W, T>(w, d); };

template<typename W, typename T>
DelegatorProxy<W, T>* decorator(W* w, T* d) { return new Decorator<W, T>(w, d); };

};
#endif
