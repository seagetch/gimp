#ifndef __SCOPE_GUARD_HPP__
#define __SCOPE_GUARD_HPP__

template<class T>
bool always_true(const T) { return true; }

template<class T, typename Destructor, Destructor* Free, bool (*IsNull)(const T) = always_true<T> >
class ScopeGuard {
protected:
  T obj;
public:
  ScopeGuard() {  };
  ScopeGuard(T ptr) : obj(ptr) {  };
  ~ScopeGuard() {
    if (!IsNull(obj)) {
      Free(obj);
    }
  }
  T& ref() { return obj; }
};



template<class T>
bool is_null(const T mem) { return !mem; };


template<class T, typename Destructor, Destructor* f>
class ScopedPointer : public ScopeGuard<T*, Destructor, f, is_null<T*> > {
public:
  ScopedPointer() : ScopeGuard<T*, Destructor, f, is_null<T*> >(NULL) {};
  ScopedPointer(T* ptr) : ScopeGuard<T*, Destructor, f, is_null<T*> >(ptr) {};
  T* ptr() { return ScopeGuard<T*, Destructor, f, is_null<T*> >::ref(); };
  T* operator ->() { return ptr(); };
  T& operator *() { return *ptr(); };
  operator T*() { return ptr(); };
  operator const T*() const { return ptr(); };
  ScopedPointer& operator =(T* src) {
    if (!is_null<T*>(this->obj))
      (*f)(this->obj);
    this->obj = src;
  }
};



template<class T>
void destroy_instance(T* instance)
{
  delete instance;
};


template<class T>
class CXXPointer : public ScopedPointer<T, void(T*), destroy_instance<T> >
{
  typedef ScopedPointer<T, void(T*), destroy_instance<T> > super;
public:
  CXXPointer() : super(NULL) { };
  CXXPointer(T* src) : super(src) { };
  CXXPointer(CXXPointer&& src) : super(src.obj) {
    src.obj = NULL;
  }
  T* operator ->() { return super::ptr(); }
  T& operator *() { return *super::ptr(); }
  operator T*() { return super::ptr(); }
  T& operator=(T* src) {
    if (!is_null(super::ptr()))
      destroy_instance(super::ptr());
    super::obj = src;
  };
  T& operator=(CXXPointer&& src) {
    g_print("steal assign %p from %p\n", src.obj, &src);
    if (!is_null(super::ptr()))
      destroy_instance(super::ptr());
    super::obj = src.obj;
    src.obj = NULL;
  }
};

template<class T>
CXXPointer<T> guard(T* ptr) { return ptr; }

template<class T>
class Nullable
{
  T* ptr;
public:
  Nullable(T* src) : ptr(src) { };
  operator const T*() { return ptr(); }
  operator T&() { if(ptr) return *ptr; return T(); }
  T& operator=(T& val) { if(ptr) *ptr = val; return val; };
  T operator=(T val) { if(ptr) *ptr = val; return val; };
};

template<class T>
Nullable<T> nullable(T* src) { return Nullable<T>(src); }
#endif
