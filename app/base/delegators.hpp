#ifndef __DELEGATORS_HPP__
#define __DELEGATORS_HPP__


#include <glib-object.h>

namespace Delegator {

template<typename T>
void destroy_cxx_object_callback(gpointer ptr) {
  delete reinterpret_cast<T*>(ptr);
}

///////////////////////////////////////////////////////////////////////////////
template<typename Function>
class Delegator {};

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename Arg0>
class Delegator_arg0 {
public:
  virtual ~Delegator_arg0() {}
  virtual Ret emit(Arg0 t) = 0;
  
  static Ret callback(Arg0 t, gpointer ptr) {
    Delegator_arg0* delegator = reinterpret_cast<Delegator_arg0*>(ptr);
    return delegator->emit(t);
  }
};

template<typename Arg0>
class Delegator_arg0<void, Arg0> {
public:
  virtual ~Delegator_arg0<void, Arg0>() {}
  virtual void emit(Arg0 t) = 0;
  
  static void callback(Arg0 t, gpointer ptr) {
    Delegator_arg0<void, Arg0>* delegator = reinterpret_cast<Delegator_arg0<void, Arg0>*>(ptr);
    delegator->emit(t);
  }
};

template<typename Ret, typename Arg0, typename Arg1>
class Delegator_arg1 {
public:
  virtual ~Delegator_arg1() {}
  virtual Ret emit(Arg0 t, Arg1 a1) = 0;
  
  static Ret callback(Arg0 t, Arg1 a1, gpointer ptr) {
    Delegator_arg1* delegator = reinterpret_cast<Delegator_arg1*>(ptr);
    return delegator->emit(t, a1);
  }
};

template<typename Arg0, typename Arg1>
class Delegator_arg1<void, Arg0, Arg1> {
public:
  virtual ~Delegator_arg1<void, Arg0, Arg1>() {}
  virtual void emit(Arg0 t, Arg1 a1) = 0;
  
  static void callback(Arg0 t, Arg1 a1, gpointer ptr) {
    Delegator_arg1<void, Arg0, Arg1>* delegator = reinterpret_cast<Delegator_arg1<void, Arg0, Arg1>*>(ptr);
    delegator->emit(t, a1);
  }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2>
class Delegator_arg2 {
public:
  virtual ~Delegator_arg2() {}
  virtual Ret emit(Arg0 t, Arg1 a1, Arg2 a2) = 0;
  
  static Ret callback(Arg0 t, Arg1 a1, Arg2 a2, gpointer ptr) {
    Delegator_arg2* delegator = reinterpret_cast<Delegator_arg2*>(ptr);
    return delegator->emit(t, a1, a2);
  }
};

template<typename Arg0, typename Arg1, typename Arg2>
class Delegator_arg2<void, Arg0, Arg1, Arg2> {
public:
  virtual ~Delegator_arg2<void, Arg0, Arg1, Arg2>() {}
  virtual void emit(Arg0 t, Arg1 a1, Arg2 a2) = 0;
  
  static void callback(Arg0 t, Arg1 a1, Arg2 a2, gpointer ptr) {
    Delegator_arg2<void, Arg0, Arg1, Arg2>* delegator = reinterpret_cast<Delegator_arg2<void, Arg0, Arg1, Arg2>*>(ptr);
    delegator->emit(t, a1, a2);
  }
};


template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
class Delegator_arg3 {
public:
  virtual ~Delegator_arg3() {}
  virtual Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3) = 0;
  
  static Ret callback(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, gpointer ptr) {
    Delegator_arg3* delegator = reinterpret_cast<Delegator_arg3*>(ptr);
    return delegator->emit(t, a1, a2, a3);
  }
};

template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
class Delegator_arg3<void, Arg0, Arg1, Arg2, Arg3> {
public:
  virtual ~Delegator_arg3<void, Arg0, Arg1, Arg2, Arg3>() {}
  virtual void emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3) = 0;
  
  static void callback(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, gpointer ptr) {
    Delegator_arg3<void, Arg0, Arg1, Arg2, Arg3>* delegator = reinterpret_cast<Delegator_arg3<void, Arg0, Arg1, Arg2, Arg3>*>(ptr);
    delegator->emit(t, a1, a2, a3);
  }
};


template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class Delegator_arg4 {
public:
  virtual ~Delegator_arg4() {}
  virtual Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) = 0;
  
  static Ret callback(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, gpointer ptr) {
    Delegator_arg4* delegator = reinterpret_cast<Delegator_arg4*>(ptr);
    return delegator->emit(t, a1, a2, a3, a4);
  }
};

template<typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class Delegator_arg4<void, Arg0, Arg1, Arg2, Arg3, Arg4> {
public:
  virtual ~Delegator_arg4<void, Arg0, Arg1, Arg2, Arg3, Arg4>() {}
  virtual void emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) = 0;
  
  static void callback(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4, gpointer ptr) {
    Delegator_arg4<void, Arg0, Arg1, Arg2, Arg3, Arg4>* delegator = reinterpret_cast<Delegator_arg4<void, Arg0, Arg1, Arg2, Arg3, Arg4>*>(ptr);
    delegator->emit(t, a1, a2, a3, a4);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename Arg0>
class Delegator<Ret (*)(Arg0)> : public Delegator_arg0<Ret, Arg0> {
};

template<typename Ret, typename Arg0, typename Arg1>
class Delegator<Ret (*)(Arg0,Arg1)> : public Delegator_arg1<Ret, Arg0, Arg1> {
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2>
class Delegator<Ret (*)(Arg0,Arg1, Arg2)> : public Delegator_arg2<Ret, Arg0, Arg1, Arg2> {
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
class Delegator<Ret (*)(Arg0,Arg1, Arg2, Arg3)> : 
  public Delegator_arg3<Ret, Arg0, Arg1, Arg2, Arg3> {
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class Delegator<Ret (*)(Ret, Arg0,Arg1, Arg2, Arg3, Arg4)> : 
  public Delegator_arg4<Ret, Arg0, Arg1, Arg2, Arg3, Arg4> {
};

///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename Arg0>
class ObjectDelegator_arg0 :
  public Delegator<Ret (*)(Arg0)> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Arg0);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator_arg0(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Arg0 t) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(t);
    if (result) return result;
    return Ret();
  }
};

template<typename Type, typename Arg0>
class ObjectDelegator_arg0<Type, void, Arg0> :
  public Delegator<void (*)(Arg0)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(Arg0);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator_arg0(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(Arg0 t) {
    if (obj)
      (obj->*func_ptr)(t);
  }
};

template<typename Type, typename Ret, typename Arg0, typename Arg1>
class ObjectDelegator_arg1 :
  public Delegator<Ret (*)(Arg0, Arg1)> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Arg0, Arg1 a1);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg1(Type* o, Function f) : obj(o), func_ptr(f) {}; 
  Ret emit(Arg0 t, Arg1 a1) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(t, a1);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Type, typename Arg0, typename Arg1>
class ObjectDelegator_arg1<Type, void, Arg0, Arg1> :
  public Delegator<void (*)(Arg0, Arg1)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(Arg0, Arg1 a1);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg1(Type* o, Function f) : obj(o), func_ptr(f) {}; 
  void emit(Arg0 t, Arg1 a1) {
    if (obj)
      (obj->*func_ptr)(t, a1);
  }
};


template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2>
class ObjectDelegator_arg2 : 
  public Delegator<Ret (*)(Arg0,Arg1, Arg2)> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Arg0, Arg1 a1, Arg2 a2);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg2(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(t, a1, a2);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Type, typename Arg0, typename Arg1, typename Arg2>
class ObjectDelegator_arg2<Type, void, Arg0, Arg1, Arg2> : 
  public Delegator<void (*)(Arg0,Arg1, Arg2)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(Arg0, Arg1 a1, Arg2 a2);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg2(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(Arg0 t, Arg1 a1, Arg2 a2) {
    if (obj)
      (obj->*func_ptr)(t, a1, a2);
  }
};

template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
class ObjectDelegator_arg3 :
  public Delegator<Ret (*)(Arg0,Arg1, Arg2, Arg3)> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Arg0, Arg1 a1, Arg2 a2, Arg3 a3);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg3(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(t, a1, a2, a3);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class ObjectDelegator_arg4 :
  public Delegator<Ret (*)(Arg0,Arg1, Arg2, Arg3, Arg4)> 
{
public:
  typedef Type type;
  typedef Ret (Type::*Function)(Arg0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg4(Type* o, Function f) : obj(o), func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) {
    Ret result = Ret();
    if (obj)
      result = (obj->*func_ptr)(t, a1, a2, a3, a4);
    if (result)
      return result;
    return Ret();
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename Arg0>
class FunctionDelegator_arg0 :
  public Delegator<Ret (*)(Arg0)> 
{
public:
  typedef Ret (*Function)(Arg0);

private:  
  Function   func_ptr;

public:
  FunctionDelegator_arg0(Function f) : func_ptr(f) {};
  Ret emit(Arg0 t) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(t);
    return Ret();
  }
};

template<typename Ret, typename Arg0, typename Arg1>
class FunctionDelegator_arg1 :
  public Delegator<Ret (*)(Arg0, Arg1)> 
{
public:
  typedef Ret (*Function)(Arg0, Arg1 a1);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg1(Function f) : func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(t, a1);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2>
class FunctionDelegator_arg2 : 
  public Delegator<Ret (*)(Arg0, Arg1, Arg2)> 
{
public:
  typedef Ret (*Function)(Arg0, Arg1 a1, Arg2 a2);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg2(Function f) : func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(t, a1, a2);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
class FunctionDelegator_arg3 :
  public Delegator<Ret (*)(Arg0,Arg1, Arg2, Arg3)> 
{
public:
  typedef Ret (*Function)(Arg0, Arg1 a1, Arg2 a2, Arg3 a3);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg3(Function f) : func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(t, a1, a2, a3);
    if (result)
      return result;
    return Ret();
  }
};

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
class FunctionDelegator_arg4 :
  public Delegator<Ret (*)(Arg0,Arg1, Arg2, Arg3, Arg4)> 
{
public:
  typedef Ret (*Function)(Arg0, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg4(Function f) : func_ptr(f) {};
  Ret emit(Arg0 t, Arg1 a1, Arg2 a2, Arg3 a3, Arg4 a4) {
    Ret result = Ret();
    if (func_ptr)
      result = (*func_ptr)(t, a1, a2, a3, a4);
    if (result)
      return result;
    return Ret();
  }
};


///////////////////////////////////////////////////////////////////////////////
template<typename Type, typename Ret, typename Arg0>
inline ObjectDelegator_arg0<Type, Ret, Arg0>* 
delegator(Type* obj,
          Ret (Type::*f)(Arg0)) 
{
  return new ObjectDelegator_arg0<Type, Ret, Arg0>(obj, f);
}

template<typename Type, typename Ret, typename Arg0, typename Arg1>
inline ObjectDelegator_arg1<Type, Ret, Arg0, Arg1>* 
delegator(Type* obj,
          Ret (Type::*f)(Arg0, Arg1)) 
{
  return new ObjectDelegator_arg1<Type, Ret, Arg0, Arg1>(obj, f);
}

template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2>
inline ObjectDelegator_arg2<Type, Ret, Arg0, Arg1, Arg2>* 
delegator(Type* obj,
          Ret (Type::*f)(Arg0, Arg1, Arg2)) 
{
  return new ObjectDelegator_arg2<Type, Ret, Arg0, Arg1, Arg2>(obj, f);
}

template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
inline ObjectDelegator_arg3<Type, Ret, Arg0, Arg1, Arg2, Arg3>* 
delegator(Type* obj,
          Ret (Type::*f)(Arg0, Arg1, Arg2, Arg3)) 
{
  return new ObjectDelegator_arg3<Type, Ret, Arg0, Arg1, Arg2, Arg3>(obj, f);
}

template<typename Type, typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline ObjectDelegator_arg4<Type, Ret, Arg0, Arg1, Arg2, Arg3, Arg4>* 
delegator(Type* obj,
          Ret (Type::*f)(Arg0, Arg1, Arg2, Arg3, Arg4)) 
{
  return new ObjectDelegator_arg4<Type, Ret, Arg0, Arg1, Arg2, Arg3, Arg4>(obj, f);
}

///////////////////////////////////////////////////////////////////////////////
template<typename Ret, typename Arg0>
inline FunctionDelegator_arg0<Ret, Arg0>* 
delegator(Ret (*f)(Arg0)) 
{
  return new FunctionDelegator_arg0<Ret, Arg0>(f);
}

template<typename Ret, typename Arg0, typename Arg1>
inline FunctionDelegator_arg1<Ret, Arg0, Arg1>* 
delegator(Ret (*f)(Arg0, Arg1)) 
{
  return new FunctionDelegator_arg1<Ret, Arg0, Arg1>(f);
}

template<typename Ret, typename Arg0, typename Arg1, typename Arg2>
inline FunctionDelegator_arg2<Ret, Arg0, Arg1, Arg2>* 
delegator(Ret (*f)(Arg0, Arg1, Arg2)) 
{
  return new FunctionDelegator_arg2<Ret, Arg0, Arg1, Arg2>(f);
}

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3>
inline FunctionDelegator_arg3<Ret, Arg0, Arg1, Arg2, Arg3>* 
delegator(Ret (*f)(Arg0, Arg1, Arg2, Arg3)) 
{
  return new FunctionDelegator_arg3<Ret, Arg0, Arg1, Arg2, Arg3>(f);
}

template<typename Ret, typename Arg0, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
inline FunctionDelegator_arg4<Ret, Arg0, Arg1, Arg2, Arg3, Arg4>* 
delegator(Ret (*f)(Arg0, Arg1, Arg2, Arg3, Arg4)) 
{
  return new FunctionDelegator_arg4<Ret, Arg0, Arg1, Arg2, Arg3, Arg4>(f);
}
///////////////////////////////////////////////////////////////////////////////
class Connection {
  GObject*  target;
  gchar*    signal;
  GClosure* closure;
public:
  Connection(GObject* target_,
             const gchar*   signal_,
             GClosure* closure_)
  {
    target  = target_;
    signal  = g_strdup(signal_);
    closure = closure_;
  };

  ~Connection()
  {
      disconnect();
  }
  
  void disconnect() {
    if (target && closure) {
      gulong handler_id = g_signal_handler_find(gpointer(target), G_SIGNAL_MATCH_CLOSURE, 0, 0, closure, NULL, NULL);
      g_signal_handler_disconnect(gpointer(target), handler_id);
    }
    target  = NULL;
    closure = NULL;
  };
  
  void block() {
    if (target && closure) {
      g_signal_handlers_block_matched (target,
                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
  }
  
  void unblock() {
    if (target && closure) {
      g_signal_handlers_unblock_matched (target,
                                       GSignalMatchType(G_SIGNAL_MATCH_DETAIL|G_SIGNAL_MATCH_CLOSURE),
                                       0, g_quark_from_static_string (signal), closure, NULL, NULL);
    }
  }
};


}; // namespace
///////////////////////////////////////////////////////////////////////////////
template<typename T>
void closure_destroy_notify(gpointer data, GClosure* closure)
{
  Delegator::destroy_cxx_object_callback<T>(data);
}

template<typename Function>
Delegator::Connection *
g_signal_connect_delegator (GObject* target, 
                            const gchar* event, 
                            Delegator::Delegator<Function>* delegator, 
                            bool after=false)
{
  GClosure *closure;  
  closure = g_cclosure_new (G_CALLBACK (Delegator::Delegator<Function>::callback),
                            (gpointer)delegator, 
                            closure_destroy_notify<Delegator::Delegator<Function> >);
  g_signal_connect_closure (target, event, closure, after);
  return new Delegator::Connection(target, event, closure);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const gchar* key, T* object)
{
  g_object_set_data_full(target, key, (gpointer)object, Delegator::destroy_cxx_object_callback<T>);
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const GQuark key, T* object)
{
  g_object_set_qdata_full(target, key, (gpointer)object, Delegator::destroy_cxx_object_callback<T>);
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
