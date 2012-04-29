#ifndef __SCOPE_GUARD_HPP__
#define __SCOPE_GUARD_HPP__

template<class T, class F>
class ScopeGuard {
  private:
  T* obj;
  F* func;
  public:
  ScopeGuard(T* ptr, F* f) : obj(ptr), func(f) {};
  ~ScopeGuard() {
    if (func && obj)
      func(obj);
  }
  T& operator *() { return *obj; }
  T& operator ->() { return *obj; }
  T* ptr() { return obj; }
};

#endif
