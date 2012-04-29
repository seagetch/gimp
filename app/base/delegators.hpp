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
class Delegator_arg0 {
public:
  virtual ~Delegator_arg0() {}
  virtual void emit(GObject* t) = 0;
  
  static void callback(GObject* t, gpointer ptr) {
    Delegator_arg0* delegator = reinterpret_cast<Delegator_arg0*>(ptr);
    delegator->emit(t);
  }
};


template<typename Arg1>
class Delegator_arg1 {
public:
  virtual ~Delegator_arg1() {}
  virtual void emit(GObject* t, Arg1 a1) = 0;
  
  static void callback(GObject* t, Arg1 a1, gpointer ptr) {
    Delegator_arg1* delegator = reinterpret_cast<Delegator_arg1*>(ptr);
    delegator->emit(t, a1);
  }
};


template<typename Arg1, typename Arg2>
class Delegator_arg2 {
public:
  virtual ~Delegator_arg2() {}
  virtual void emit(GObject* t, Arg1 a1, Arg2 a2) = 0;
  
  static void callback(GObject* t, Arg1 a1, Arg2 a2, gpointer ptr) {
    Delegator_arg2* delegator = reinterpret_cast<Delegator_arg2*>(ptr);
    delegator->emit(t, a1, a2);
  }
};


template<typename Arg1, typename Arg2, typename Arg3>
class Delegator_arg3 {
public:
  virtual ~Delegator_arg3() {}
  virtual void emit(GObject* t, Arg1 a1, Arg2 a2, Arg3 a3) = 0;
  
  static void callback(GObject* t, Arg1 a1, Arg2 a2, Arg3 a3, gpointer ptr) {
    Delegator_arg3* delegator = reinterpret_cast<Delegator_arg3*>(ptr);
    delegator->emit(t, a1, a2, a3);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<>
class Delegator<void (*)(GObject*)> : public Delegator_arg0 {
};

template<typename Arg1>
class Delegator<void (*)(GObject*,Arg1)> : public Delegator_arg1<Arg1> {
};

template<typename Arg1, typename Arg2>
class Delegator<void (*)(GObject*,Arg1, Arg2)> : public Delegator_arg2<Arg1, Arg2> {
};

template<typename Arg1, typename Arg2, typename Arg3>
class Delegator<void (*)(GObject*,Arg1, Arg2, Arg3)> : 
  public Delegator_arg3<Arg1, Arg2, Arg3> {
};

///////////////////////////////////////////////////////////////////////////////
template<typename Type>
class ObjectDelegator_arg0 :
  public Delegator<void (*)(GObject*)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(GObject*);

private:  
  Type*    obj;
  Function   func_ptr;

public:
  ObjectDelegator_arg0(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(GObject* t) {
    if (obj)
      (obj->*func_ptr)(t);
  }
};

template<typename Type, typename Arg1>
class ObjectDelegator_arg1 :
  public Delegator<void (*)(GObject*, Arg1)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(GObject*, Arg1 a1);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg1(Type* o, Function f) : obj(o), func_ptr(f) {}; 
  void emit(GObject* t, Arg1 a1) {
    if (obj)
      (obj->*func_ptr)(t, a1);
  }
};

template<typename Type, typename Arg1, typename Arg2>
class ObjectDelegator_arg2 : 
  public Delegator<void (*)(GObject*,Arg1, Arg2)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(GObject*, Arg1 a1, Arg2 a2);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg2(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(GObject* t, Arg1 a1, Arg2 a2) {
    g_print("emit:obj=%ld\n", obj);
    if (obj)
      (obj->*func_ptr)(t, a1, a2);
  }
};

template<typename Type, typename Arg1, typename Arg2, typename Arg3>
class ObjectDelegator_arg3 :
  public Delegator<void (*)(GObject*,Arg1, Arg2, Arg3)> 
{
public:
  typedef Type type;
  typedef void (Type::*Function)(GObject*, Arg1 a1, Arg2 a2, Arg3 a3);

private:  
  Type*    obj;
  Function func_ptr;

public:
  ObjectDelegator_arg3(Type* o, Function f) : obj(o), func_ptr(f) {};
  void emit(GObject* t, Arg1 a1, Arg2 a2, Arg3 a3) {
    if (obj)
      (obj->*func_ptr)(t, a1, a2, a3);
  }
};

///////////////////////////////////////////////////////////////////////////////
class FunctionDelegator_arg0 :
  public Delegator<void (*)(GObject*)> 
{
public:
  typedef void (*Function)(GObject*);

private:  
  Function   func_ptr;

public:
  FunctionDelegator_arg0(Function f) : func_ptr(f) {};
  void emit(GObject* t) {
    if (func_ptr)
      (*func_ptr)(t);
  }
};

template<typename Arg1>
class FunctionDelegator_arg1 :
  public Delegator<void (*)(GObject*, Arg1)> 
{
public:
  typedef void (*Function)(GObject*, Arg1 a1);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg1(Function f) : func_ptr(f) {};
  void emit(GObject* t, Arg1 a1) {
    if (func_ptr)
      (*func_ptr)(t, a1);
  }
};

template<typename Arg1, typename Arg2>
class FunctionDelegator_arg2 : 
  public Delegator<void (*)(GObject*,Arg1, Arg2)> 
{
public:
  typedef void (*Function)(GObject*, Arg1 a1, Arg2 a2);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg2(Function f) : func_ptr(f) {};
  void emit(GObject* t, Arg1 a1, Arg2 a2) {
    if (func_ptr)
      (*func_ptr)(t, a1, a2);
  }
};

template<typename Arg1, typename Arg2, typename Arg3>
class FunctionDelegator_arg3 :
  public Delegator<void (*)(GObject*,Arg1, Arg2, Arg3)> 
{
public:
  typedef void (*Function)(GObject*, Arg1 a1, Arg2 a2, Arg3 a3);

private:  
  Function func_ptr;

public:
  FunctionDelegator_arg3(Function f) : func_ptr(f) {};
  void emit(GObject* t, Arg1 a1, Arg2 a2, Arg3 a3) {
    if (func_ptr)
      (*func_ptr)(t, a1, a2, a3);
  }
};

///////////////////////////////////////////////////////////////////////////////
template<typename Type>
inline ObjectDelegator_arg0<Type>* 
delegator(Type* obj,
          void (Type::*f)(GObject*)) 
{
  return new ObjectDelegator_arg0<Type>(obj, f);
}

template<typename Type, typename Arg1>
inline ObjectDelegator_arg1<Type, Arg1>* 
delegator(Type* obj,
          void (Type::*f)(GObject*, Arg1)) 
{
  return new ObjectDelegator_arg1<Type, Arg1>(obj, f);
}

template<typename Type, typename Arg1, typename Arg2>
inline ObjectDelegator_arg2<Type, Arg1, Arg2>* 
delegator(Type* obj,
          void (Type::*f)(GObject*, Arg1, Arg2)) 
{
  return new ObjectDelegator_arg2<Type, Arg1, Arg2>(obj, f);
}

template<typename Type, typename Arg1, typename Arg2, typename Arg3>
inline ObjectDelegator_arg3<Type,Arg1, Arg2, Arg3>* 
delegator(Type* obj,
          void (Type::*f)(GObject*, Arg1, Arg2, Arg3)) 
{
  return new ObjectDelegator_arg3<Type, Arg1, Arg2, Arg3>(obj, f);
}

///////////////////////////////////////////////////////////////////////////////
inline FunctionDelegator_arg0* 
delegator(void (*f)(GObject*)) 
{
  return new FunctionDelegator_arg0(f);
}

template<typename Arg1>
inline FunctionDelegator_arg1<Arg1>* 
delegator(void (*f)(GObject*, Arg1)) 
{
  return new FunctionDelegator_arg1<Arg1>(f);
}

template<typename Arg1, typename Arg2>
inline FunctionDelegator_arg2<Arg1, Arg2>* 
delegator(void (*f)(GObject*, Arg1, Arg2)) 
{
  return new FunctionDelegator_arg2<Arg1, Arg2>(f);
}

template<typename Arg1, typename Arg2, typename Arg3>
inline FunctionDelegator_arg3<Arg1, Arg2, Arg3>* 
delegator(void (*f)(GObject*, Arg1, Arg2, Arg3)) 
{
  return new FunctionDelegator_arg3<Arg1, Arg2, Arg3>(f);
}
};


///////////////////////////////////////////////////////////////////////////////
template<typename T>
void closure_destroy_notify(gpointer data, GClosure* closure)
{
  Delegator::destroy_cxx_object_callback<T>(data);
}

template<typename Function>
GClosure *
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
  return closure;
}

template<typename T>
inline void g_object_set_cxx_object (GObject* target, const gchar* key, T* object)
{
  g_object_set_data_full(target, key, (gpointer)object, Delegator::destroy_cxx_object_callback<T>);
}

#endif
