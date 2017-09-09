#ifndef __DELEGATORS_HPP__
#define __DELEGATORS_HPP__

extern "C" {
#include <glib-object.h>
};
#include <utility>
#include <functional>

namespace Function {

template<typename F>
struct FunctionBase { };

template<typename Ret, typename... Args>
struct FunctionBase<Ret(Args...)> {
  template<typename F>
  static Ret callback(Args... args, F* f) {
    return (*f)(args...);
  }
};

template<typename Decl, typename F>
struct Function {
  using Callback = decltype(&FunctionBase<Decl>::template callback<F>);
  Callback callback;
  F f;
};

template<typename F, typename Ret, typename... Args>
struct Function<Ret(Args...), F> {
  using Callback = decltype(&FunctionBase<Ret(Args...)>::template callback<F>);
  Callback callback;
  F f;
  Ret operator() (Args... args) {
    (*callback)(args..., &f);
  }
};

template<typename Decl, typename F>
auto function(F f) {
  Function<Decl, F> holder = {
      &FunctionBase<Decl>::template callback<F>,
      f
  };
  return holder;
};

};

namespace Delegators {
template<typename T>
void destroy_cxx_object_callback(gpointer ptr) {
  delete reinterpret_cast<T*>(ptr);
}

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class Delegator {
  bool enabled;
public:
  Delegator() : enabled(true) {}
  virtual ~Delegator() {}
  virtual Ret emit(Args... args) = 0;

  void enable() { enabled = true; }
  void disable() { enabled = false; }
  bool is_enabled() { return enabled; }
  
  static Ret callback(Args... args, gpointer ptr) {
    Delegator* delegator = reinterpret_cast<Delegator*>(ptr);
    if (delegator->is_enabled())
      return delegator->emit(args...);
    return Ret();
  }

  typedef Ret (*Callback)(Args..., gpointer);
  Callback _callback() {
    return &callback;
  };

};
///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename... Args>
class ObjectDelegator : public Delegator<Ret, Args...> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Args...);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Args... args) {
    if (obj)
      return (obj->*func_ptr)(args...);
    return Ret();
  }
};
///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class FunctionDelegator :
  public Delegator<Ret, Args...> 
{
public:
  typedef Ret (*Function)(Args...);

private:  
  Function   func_ptr;

public:
  FunctionDelegator(Function f) : func_ptr(f) {};
  Ret emit(Args... args) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(args...);
    return result;
  }
};
///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename... Args>
class CXXFunctionDelegator :
  public Delegator<Ret, Args...> 
{
public:
  typedef std::function<Ret (Args...)> Function;

private:  
  Function   func_ref;

public:
  CXXFunctionDelegator(Function f) : func_ref(f) {};
  Ret emit(Args... args) {
    if (func_ref)
      return func_ref(args...);
    return Ret();
  }
};
///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename... Args>
inline ObjectDelegator<Type, Ret, Args...>* 
delegator(Type* obj, Ret (Type::*f)(Args...)) 
{
  return new ObjectDelegator<Type, Ret, Args...>(obj, f);
}

template<typename Ret, typename... Args>
inline FunctionDelegator<Ret, Args...>* 
delegator(Ret (*f)(Args...)) 
{
  return new FunctionDelegator<Ret, Args...>(f);
}

template<typename Ret, typename... Args>
inline CXXFunctionDelegator<Ret, Args...>* 
delegator(std::function<Ret (Args...)>&& f)
{
  return new CXXFunctionDelegator<Ret, Args...>(std::move(f));
}

///////////////////////////////////////////////////////////////////////////////
class Connection {
  gulong    handler_id;
  GObject*  target;
  gchar*    signal;
  GClosure* closure;
public:
  Connection(gulong handler_id_,
             GObject* target_,
             const gchar*   signal_,
             GClosure* closure_)
  {
    handler_id = handler_id_;
    target  = target_;
    signal  = g_strdup(signal_);
    closure = closure_;
  };

  ~Connection()
  {
      disconnect();
  }
  
  bool is_valid() { return handler_id > 0; }

  void disconnect() {
    if (target && closure) {
      gulong handler_id = g_signal_handler_find(gpointer(target), G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
      if (handler_id > 0)
        g_signal_handler_disconnect(gpointer(target), handler_id);
    }
    target  = NULL;
    closure = NULL;
  };
  
  void block() {
    if (target && closure) {
//      g_signal_handlers_block_matched (target,
//                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
//                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
    if (target)
      g_signal_handler_block (target, handler_id);
  }
  
  void unblock() {
    if (target && closure) {
//      g_signal_handlers_unblock_matched (target,
//                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
//                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
    if (target)
      g_signal_handler_unblock (target, handler_id);
  }
};


}; // namespace
///////////////////////////////////////////////////////////////////////////////
template<typename T>
void closure_destroy_notify(gpointer data, GClosure* closure)
{
  Delegators::destroy_cxx_object_callback<T>(data);
}

template<typename Ret, typename... Args>
Delegators::Connection *
g_signal_connect_delegator (GObject* target, 
                            const gchar* event, 
                            Delegators::Delegator<Ret, Args...>* delegator, 
                            bool after=false)
{
  GClosure *closure;  
  closure = g_cclosure_new (G_CALLBACK (delegator->_callback()),
                            (gpointer)delegator, 
                            closure_destroy_notify<Delegators::Delegator<Ret, Args...> >);
  gulong handler_id = g_signal_connect_closure (target, event, closure, after);
  return new Delegators::Connection(handler_id, target, event, closure);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const gchar* key, T* object)
{
  g_object_set_data_full(target, key, (gpointer)object, Delegators::destroy_cxx_object_callback<T>);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const GQuark key, T* object)
{
  g_object_set_qdata_full(target, key, (gpointer)object, Delegators::destroy_cxx_object_callback<T>);
}

inline gulong
g_signal_handler_get_id(GObject* target, GClosure* closure)
{
  return g_signal_handler_find(gpointer(target), G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
}

inline void
g_signal_disconnect (GObject* target,
                     GClosure* closure)
{
  gulong handler_id = g_signal_handler_get_id(target, closure);
	g_signal_handler_disconnect(gpointer(target), handler_id);
}

inline void
g_signal_disconnect (GObject* target,
                     gulong handler_id)
{
	g_signal_handler_disconnect(gpointer(target), handler_id);
}

#endif
