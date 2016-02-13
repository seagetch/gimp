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
    g_value_init(&result, pspec->value_type);
    g_object_get_property(object, prop_name, &result);
    return result;
  };

  void set(const gchar* prop_name, GValueWrapper value) {
    GObject* object = as_object();
    g_object_set_property(object, prop_name, &value.ref());
  };

  template<typename Ret, typename... Args>
  Delegator::Connection* connect(const gchar* event, 
            Delegator::Delegator<Ret, Args...>* delegator, 
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
  operator GTypeInstance* () { return reinterpret_cast<GTypeInstance*>(super::ptr()); }
  T* operator ->() { return super::ptr(); };
  operator bool() { return super::ptr(); };

  void operator = (T* src) {
    if (!is_null(super::obj))
      super::decref();
    super::obj = src;
    super::incref();
  };
  
  template<typename Ret, typename... Args>
  class BoundMethodBase {
  protected:
    typedef Ret Method (T*o, Args...);
    Method * method;
    T* obj;
  public:
    BoundMethodBase(T* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) { return (*method)(obj, args...); };
  };

  template<typename Ret, typename... Args>
  class BoundMethod : public BoundMethodBase<Ret, Args...> {
    typedef Ret Method (T*o, Args...);
  public:
    BoundMethod(T* o, Method* m) : BoundMethodBase<Ret, Args...>(o, m) {};
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename Ret, typename... Args>
  class BoundMethod<Ret*, Args...> : public BoundMethodBase<Ret*, Args...> {
    typedef Ret* Method (T*o, Args...);
  public:
    BoundMethod(T* o, Method* m) : BoundMethodBase<Ret*, Args...>(o, m) {};
    GWrapper<Ret> operator ()(Args... args) { return GWrapper<Ret>((*this->method)(this->obj, args...)); };
  };

  template<typename... Args>
  class BoundMethod<void, Args...> : public BoundMethodBase<void, Args...> {
    typedef void Method (T*o, Args...);
  public:
    BoundMethod(T* o, Method* m) : BoundMethodBase<void, Args...>(o, m) {};
    void call(Args... args) { (*this->method)(this->obj, args...); };
    void operator ()(Args... args) { call(args...); };
  };
  
  template<typename Ret, typename... Args>
  class ConstBoundMethodBase {
  protected:
    typedef Ret Method (const T*o, Args...);
    Method * method;
    const T* obj;
  public:
    ConstBoundMethodBase(const T* o, Method* m) : obj(o), method(m) {};
    Ret call(Args... args) { return (*method)(obj, args...); };
  };

  template<typename Ret, typename... Args>
  class ConstBoundMethod : public ConstBoundMethodBase<Ret, Args...> {
    typedef Ret Method (const T*o, Args...);
  public:
    ConstBoundMethod(const T* o, Method* m) : ConstBoundMethodBase<Ret, Args...>(o, m) {};
    Ret operator ()(Args... args) { return this->call(args...); };
  };

  template<typename Ret, typename... Args>
  class ConstBoundMethod<Ret*, Args...> : public ConstBoundMethodBase<Ret*, Args...> {
    typedef Ret* Method (const T*o, Args...);
  public:
    ConstBoundMethod(const T* o, Method* m) : ConstBoundMethodBase<Ret*, Args...>(o, m) {};
    GWrapper<Ret> operator ()(Args... args) { return GWrapper<Ret>((*this->method)(this->obj, args...)); };
  };
  
  template<typename... Args>
  class ConstBoundMethod<void, Args...> : public ConstBoundMethodBase<void, Args...> {
    typedef void Method (const T*o, Args...);
  public:
    ConstBoundMethod(const T* o, Method* m) : ConstBoundMethodBase<void, Args...>(o, m) {};
    void call(Args... args) { (*this->method)(this->obj, args...); };
    void operator()(Args... args) { call(args...); }
  };

  template<typename Ret, typename... Args>
  BoundMethod<Ret, Args...> operator []( Ret f(T*, Args...) ) {
    return BoundMethod<Ret, Args...>(super::obj, f);
  };

  template<typename Ret, typename... Args>
  ConstBoundMethod<Ret, Args...> operator []( Ret f(const T*, Args...) ) {
    return ConstBoundMethod<Ret, Args...>(super::obj, f);
  };

  GValueWrapper operator[](const gchar* name) { return this->get(name); }
};

template<class T>
inline GWrapper<T> 
_G(T* obj) {
  return GWrapper<T>(obj);
}

template<typename W, typename T>
class DelegatorProxy {
protected:
  W* widget;
  T* obj;
public:
  DelegatorProxy(W* w, T* d) : widget(w), obj(d) { };

  template<typename F>
  DelegatorProxy connect(const gchar* name, F f) {
    g_signal_connect_delegator (G_OBJECT(widget), name, Delegator::delegator(obj, f));
    return *this;
  };
  template<typename F>
  Delegator::Connection* connecth(const gchar* name, F f) {
    return g_signal_connect_delegator (G_OBJECT(widget), name, Delegator::delegator(obj, f));
  };
};

template<typename W, typename T>
class Decorator : public DelegatorProxy<W, T> {
public:
  Decorator(W* w, T* d) : DelegatorProxy<W, T>(w, d) {
    StringHolder attr = g_strdup_printf("_decorator%lx", (gulong)this->widget);
    g_object_set_cxx_object(G_OBJECT(this->widget), attr.ptr(), this->obj);
  };
};

template<typename W, typename T>
DelegatorProxy<W, T> delegator(W* w, T* d) { return DelegatorProxy<W, T>(w, d); };

template<typename W, typename T>
DelegatorProxy<W, T> decorator(W* w, T* d) { return Decorator<W, T>(w, d); };
#endif
