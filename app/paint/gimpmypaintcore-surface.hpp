/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GIMP_MYPAINT_CORE_SURFACE_HPP__
#define __GIMP_MYPAINT_CORE_SURFACE_HPP__

extern "C" {
#include <glib-object.h>
#include "core/gimpobject.h"
#include "gimpmypaintoptions.h"
#include "core/gimpmypaintbrush.h"
#include "core/gimpundo.h"
#include "core/gimpbrush.h"
}

extern "C++" {
#include "mypaintbrush-surface.hpp"
#define REAL_CALC
#include "core/gimpcoords.h"
#include "base/pixel.hpp"

class GimpMypaintSurface : public Surface
{
public:

  virtual bool is_surface_for (GimpDrawable* drawable) = 0;
  virtual void set_bg_color (GimpRGB* src) = 0;
  virtual void get_bg_color(GimpRGB* dest) = 0;
  virtual void set_brushmark(GimpBrush* brush_) = 0;
  virtual GimpBrush* get_brushmark() = 0;
  virtual void set_floating_stroke(bool value) = 0;
  virtual bool get_floating_stroke() = 0;
  virtual void set_stroke_opacity(double value) = 0;
  virtual void set_coords(const GimpCoords* coords) = 0;
  virtual void set_texture(GimpPattern* texture) = 0;
  virtual GimpPattern* get_texture() = 0;
};

GimpMypaintSurface* GimpMypaintSurface_new(GimpDrawable* drawable);
GimpMypaintSurface* GimpMypaintSurface_TempBuf_new(TempBuf* drawable);

}
#endif  /*  __GIMP_MYPAINT_SURFACE_HPP__  */
