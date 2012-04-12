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
#include "core/gimpobject.h"
#include "gimpmypaintoptions.h"
#include "core/gimpmypaintbrush.h"
#include "core/gimpundo.h"
}

extern "C++" {
#include "mypaintbrush-surface.hpp"
#include "base/pixel.hpp"

class GimpMypaintSurface : public Surface
{
#if 0
  virtual TempBuf* get_paint_area (GimpDrawable *drawable,
                                    GimpMypaintOptions *options,
                                    const GimpCoords *coords);
#endif
  virtual GimpUndo* push_undo (GimpImage *imag, const gchar* undo_desc);
  
  GimpDrawable* drawable;
  
  gchar       *undo_desc;        /*  undo description                    */
  TileManager* undo_tiles;       /*  tiles which have been modified      */

  gint         x1, y1;           /*  undo extents in image coords        */
  gint         x2, y2;           /*  undo extents in image coords        */
  gint         session;          /*  reference counter of atomic scope   */
#if 0
  TempBuf * get_orig_image            (gint              x,
                                       gint              y,
                                       gint              width,
                                       gint              height);
  TempBuf * get_orig_proj             (GimpPickable     *pickable,
                                       gint              x,
                                       gint              y,
                                       gint              width,
                                       gint              height);
#endif
  void      paste                     (PixelRegion              *mypaint_maskPR,
                                       gdouble                   mypaint_opacity,
                                       gdouble                   image_opacity,
                                       GimpLayerModeEffects      mypaint_mode,
                                       GimpMypaintBrushMode      mode);
  void      replace                   (PixelRegion              *mypaint_maskPR,
                                       gdouble                   mypaint_opacity,
                                       gdouble                   image_opacity,
                                       GimpMypaintBrushMode  mode);

  void      validate_undo_tiles       (gint              x,
                                       gint              y,
                                       gint              w,
                                       gint              h);
#if 0
  void      validate_canvas_tiles     (GimpMypaintCore    *core,
                                       gint              x,
                                       gint              y,
                                       gint              w,
                                       gint              h);
#endif
  void      copy_valid_tiles          (TileManager *src_tiles,
                                       TileManager *dest_tiles,
                                       gint         x,
                                       gint         y,
                                       gint         w,
                                       gint         h);

  void render_dab_mask_in_tile (Pixel::data_t * mask,
                                float x, float y,
                                float radius,
                                float hardness,
                                float aspect_ratio, float angle,
                                int w, int h, gint stride
                                );
public:
  GimpMypaintSurface(GimpDrawable* drawable);
  virtual ~GimpMypaintSurface();

  virtual bool draw_dab (float x, float y, 
                         float radius, 
                         float color_r, float color_g, float color_b,
                         float opaque, float hardness = 0.5,
                         float alpha_eraser = 1.0,
                         float aspect_ratio = 1.0, float angle = 0.0,
                         float lock_alpha = 0.0, float colorize = 0.0
                         );

  virtual void get_color (float x, float y, 
                          float radius, 
                          float * color_r, float * color_g, float * color_b, float * color_a
                          );

  virtual void begin_session();
  virtual GimpUndo* end_session();
  
  bool is_surface_for (GimpDrawable* drawable) { return drawable == this->drawable; }
};

}
#endif  /*  __GIMP_MYPAINT_SURFACE_HPP__  */
