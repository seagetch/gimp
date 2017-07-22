#ifndef __GLIB_CXX_UTILS_HPP__
#define __GLIB_CXX_UTILS_HPP__

extern "C" {
#include <glib.h>
#include "core/core-types.h"
#include "core/gimpparamspecs.h"
}

#include "base/scopeguard.hpp"
#include "base/delegators.hpp"
#include "base/glib-cxx-impl.hpp"

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
public:
  StringList() : ScopedPointer<gchar*, void(gchar**), g_strfreev>(NULL) {};
  StringList(gchar** list) : ScopedPointer<gchar*, void(gchar**), g_strfreev>(list) {};
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

template<typename Key, typename Data>
class HashTable : public ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>
{
  template<typename F>
  static void foreach_callback(gpointer key, gpointer data, gpointer user) {
    F* func = reinterpret_cast<F*>(user);
    (*func)((Key)key, (Data)data);
  };
public:
  HashTable(GHashTable* table) : ScopedPointer<GHashTable, void(GHashTable*), g_hash_table_unref>(table) {};

  const Data lookup(const Key key) const {
    return (Data)(g_hash_table_lookup(obj, (const gpointer) key));
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
};

class Regex : public ScopedPointer<GRegex, void(GRegex*), g_regex_unref>
{
public:
  class Matched : public ScopedPointer<GMatchInfo, void(GMatchInfo*), g_match_info_unref> {
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

class Value : ScopedPointer<GValue*, void(GValue*), g_value_unset>
{
public:
  Value(GValue* src) : ScopedPointer<GValue*, void(GValue*), g_value_unset>(src) {};
  Value(Value&& src) : ScopedPointer<GValue*, void(GValue*), g_value_unset>(src.obj) {
    src.obj = NULL;
  };
  GType type() { return G_VALUE_TYPE(obj); };
  operator const gchar() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_CHAR);
    g_value_transform(obj, &value);
    return g_value_get_schar(&value);
  };
  operator const bool() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_BOOLEAN);
    g_value_transform(obj, &value);
    return g_value_get_boolean(&value);
  };
  operator const gint() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_INT);
    g_value_transform(obj, &value);
    return g_value_get_int(&value);
  };
  operator const guint() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_UINT);
    g_value_transform(obj, &value);
    return g_value_get_uint(&value);
  };
  operator const glong() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_LONG);
    g_value_transform(obj, &value);
    return g_value_get_long(&value);
  };
  operator const gulong() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_ULONG);
    g_value_transform(obj, &value);
    return g_value_get_ulong(&value);
  };
  operator const gfloat() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_FLOAT);
    g_value_transform(obj, &value);
    return g_value_get_float(&value);
  };
  operator const gdouble() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_DOUBLE);
    g_value_transform(obj, &value);
    return g_value_get_double(&value);
  };
  operator const gchar*() const {
    GValue value = G_VALUE_INIT;
    g_value_init(&value, G_TYPE_STRING);
    g_value_transform(obj, &value);
    return g_value_get_string(&value);
  };
  operator const GObject*() const {
    return G_OBJECT(g_value_get_object(obj));
  };
};

inline Value _G(GValue* src) { return Value(src); }

class Variant
{
private:
  GValue self;
  Value wrapper;
public:
  Variant(const GValue& src) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_VALUE_TYPE(&src));
    g_value_copy(&src, &self);
  };
  Variant(gchar val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_CHAR);
    g_value_set_schar(&self, val);
  };
  Variant(bool val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_BOOLEAN);
    g_value_set_boolean(&self, val);
  };

  Variant(gint32 val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, GIMP_TYPE_INT32);
    g_value_set_int(&self, val);
  };
  Variant(guint32 val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_UINT);
    g_value_set_int(&self, val);
  };
  Variant(glong val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_LONG);
    g_value_set_long(&self, val);
  };
  Variant(gulong val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_ULONG);
    g_value_set_ulong(&self, val);
  };
  Variant(gfloat val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_FLOAT);
    g_value_set_float(&self, val);
  };
  Variant(gdouble val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_DOUBLE);
    g_value_set_double(&self, val);
  };
  Variant(GObject* val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_OBJECT);
    g_value_set_object(&self, val);
  };
  Variant(gchar* val) : wrapper(&self) {
    GValue default_value = G_VALUE_INIT;
    self = default_value;
    g_value_init(&self, G_TYPE_STRING);
    g_value_set_string(&self, val);
  };
  GValue& ref() { return self; };
  GType type() { return G_VALUE_TYPE(&self); };

  operator const gchar() const { return gchar(wrapper); };
  operator const bool() const { return bool(wrapper); };
  operator const gint() const { return gint(wrapper); };
  operator const guint() const { return guint(wrapper); };
  operator const glong() const { return glong(wrapper); };
  operator const gulong() const { return gulong(wrapper); };
  operator const gfloat() const { return gfloat(wrapper); };
  operator const gdouble() const { return gdouble(wrapper); };
  operator const gchar*() const { return (const gchar*)wrapper; };
  operator const GObject*() const { return (const GObject*)wrapper; };
};

inline Variant _G(const GValue& src) { return Variant(src); }

class GListHolder : public ScopedPointer<GList, void(GList*), g_list_free>
{
public:
  GListHolder(GList* list) : ScopedPointer<GList, void(GList*), g_list_free>(list) {};
};

template<class T>
class GListWrapper : public GListHolder {
public:
  GListWrapper(GList* list) : GListHolder(list) { };
  T operator[](int index) {
    return T(g_list_nth_data(obj, index));
  }
};

template<class T>
class Object : public ScopedPointer<T, void(gpointer), g_object_unref>
{
  typedef ScopedPointer<T, void(gpointer), g_object_unref> super;
public:
  Object() : super(NULL) {};
  Object(T* object) : super(object) {};

  GObject* as_object() { return G_OBJECT(super::obj); }
  const GObject* as_object() const { return G_OBJECT(super::obj); }
  void incref() { g_object_ref(as_object()); };
  void decref() { g_object_unref(as_object()); };
  void freeze() { g_object_freeze_notify(as_object()); };
  void thaw()   { g_object_thaw_notify(as_object()); };

  struct InvalidIndex { const gchar* name; InvalidIndex(const gchar* n) : name(n) {;}; };
  
  Variant get(const gchar* prop_name) {
    GObject* object = as_object();
    GObjectClass* class_obj = G_OBJECT_GET_CLASS(object);
    GParamSpec *pspec = g_object_class_find_property (class_obj, prop_name);
    if (!pspec) throw InvalidIndex(prop_name);
    GValue result = G_VALUE_INIT;
    g_value_init(&result, pspec->value_type);
    g_object_get_property(object, prop_name, &result);
    return result;
  };

  void set(const gchar* prop_name, Variant&& value) {
    GObject* object = as_object();
    g_object_set_property(object, prop_name, &value.ref());
  };

  template<typename Ret, typename... Args>
  Delegators::Connection* connect(const gchar* event,
            Delegators::Delegator<Ret, Args...>* delegator,
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

template<typename T>
class ObjectWrapper : public Object<T>
{
  typedef Object<T> super;
public:
  ObjectWrapper() : super(NULL) { };
  ObjectWrapper(T* object) : super(object) {
    super::incref();
  };
  ObjectWrapper(const ObjectWrapper& src) : super (src.obj) {
    super::incref();
  }

  operator GObject* () { return super::as_object(); };
  operator T* () { return super::ptr(); };
  template<typename G>
  operator G* () { return GLib::cast<G>(super::obj); };
  operator GTypeInstance* () { return GLib::cast<GTypeInstance>(super::ptr()); }
  T* operator ->() { return super::ptr(); };
  operator bool() { return super::ptr(); };

  void operator = (T* src) {
    if (!is_null(super::obj))
      super::decref();
    super::obj = src;
    super::incref();
  };
  
  template<typename Ret, typename C, typename... Args>
  class BoundMethodBase {
  protected:
    typedef Ret Method (C*o, Args...);
    Method * method;
    C* obj;
  public:
    BoundMethodBase(C* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) { return (*method)(obj, args...); };
  };

  template<typename Ret, typename C, typename... Args>
  class BoundMethod : public BoundMethodBase<Ret, C, Args...> {
    typedef Ret Method (C*o, Args...);
  public:
    BoundMethod(C* o, Method* m) : BoundMethodBase<Ret, C, Args...>(o, m) {};
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename C, typename... Args>
  class BoundMethod<void, C, Args...> : public BoundMethodBase<void, C, Args...> {
    typedef void Method (C*o, Args...);
  public:
    BoundMethod(C* o, Method* m) : BoundMethodBase<void, C, Args...>(o, m) {};
    void call(Args... args) { (*this->method)(this->obj, args...); };
    void operator ()(Args... args) { call(args...); };
  };
  
  template<typename Ret, typename C, typename... Args>
  class ConstBoundMethodBase {
  protected:
    typedef Ret Method (const C*o, Args...);
    Method * method;
    const C* obj;
  public:
    ConstBoundMethodBase(const C* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) { return (*method)(obj, args...); };
  };

  template<typename Ret, typename C, typename... Args>
  class ConstBoundMethod : public ConstBoundMethodBase<Ret, C, Args...> {
    typedef Ret Method (const C*o, Args...);
  public:
    ConstBoundMethod(const C* o, Method* m) : ConstBoundMethodBase<Ret, C, Args...>(o, m) {};
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename C, typename... Args>
  class ConstBoundMethod<void, C, Args...> : public ConstBoundMethodBase<void, C, Args...> {
    typedef void Method (const C*o, Args...);
  public:
    ConstBoundMethod(const C* o, Method* m) : ConstBoundMethodBase<void, C, Args...>(o, m) {};
    void call(Args... args) { (*this->method)(this->obj, args...); };
    void operator()(Args... args) { call(args...); }
  };

  template<typename Ret, typename C, typename... Args>
  BoundMethod<Ret, C, Args...> operator []( Ret f(C*, Args...) ) {
    return BoundMethod<Ret, C, Args...>(GLib::cast<C>(super::obj), f);
  };

  template<typename Ret, typename C, typename... Args>
  ConstBoundMethod<Ret, C, Args...> operator []( Ret f(const C*, Args...) ) {
    return ConstBoundMethod<Ret, C, Args...>(GLib::cast<C>(super::obj), f);
  };

  class GValueAssigner {
    ObjectWrapper* owner;
    const gchar* name;
  public:
    GValueAssigner(ObjectWrapper* o, const gchar* n) : owner(o), name(n) { };
    void operator =(const Variant&& value) { owner->set(name, value); }
  };
//  GValueAssigner operator [](const gchar* name) { return GValueAssigner(this); }
  Variant operator[] (const gchar* name) { return this->get(name); }
  template<typename D>
  Delegators::Connection* connect(const gchar* signal_name, D d) {
    return g_signal_connect_delegator(G_OBJECT(super::obj), signal_name, d);
  }
};

template<class T>
inline ObjectWrapper<T> _G(T* obj) {
  return ObjectWrapper<T>(obj);
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
  };
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
