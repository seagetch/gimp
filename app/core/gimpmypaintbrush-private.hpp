#ifndef __GIMP_MYPAINT_BRUSH_PRIVATE_H__
#define __GIMP_MYPAINT_BRUSH_PRIVATE_H__

#include <glib.h>
#include "mypaintbrush-mapping.hpp"
#include "mypaintbrush-enum-settings.h"

extern "C++" {

class GimpMypaintBrushPrivate {
  public:
  struct Value {
    float base_value;
    Mapping *mapping;
  };
  private:
  char *parent_brush_name;
  char *group;
  Value  settings[BRUSH_SETTINGS_COUNT];

  public:
  GimpMypaintBrushPrivate();
  ~GimpMypaintBrushPrivate();
  Value* get_setting(int index) {
    g_assert (0 <= index && index < BRUSH_SETTINGS_COUNT);
    return &settings[index];
  }
  void set_base_value (int index, float value);
  void allocate_mapping (int index);
  void deallocate_mapping (int index);
  char* get_parent_brush_name();
  void  set_parent_brush_name(char *name);
  char* get_group();
  void  set_group(char *name);
};

void destroy_gimp_mypaint_brush_private(gpointer data);
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

}
#endif
