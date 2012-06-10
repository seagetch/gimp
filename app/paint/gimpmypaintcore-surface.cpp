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

extern "C" {
#include "config.h"

#include <string.h>
#include <math.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "paint-types.h"

#include "base/base-types.h"
#include "base/pixel-region.h"
#include "base/temp-buf.h"
#include "base/tile-manager.h"
#include "base/tile.h"
#include "base/pixel-processor.h"

#include "core/gimp.h"
#include "core/gimp-utils.h"
#include "core/gimpdrawable.h"
#include "core/gimpimage.h"
#include "core/gimpimage-undo.h"
#include "core/gimpimage-undo-push.h"
#include "core/gimpchannel.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"

#include "gimpmypaintcore.hpp"
#include "gimpmypaintcoreundo.h"
#include "gimpmypaintoptions.h"

#include "gimp-intl.h"
};

#define REAL_CALC

#include "base/pixel.hpp"
#include "base/scopeguard.hpp"
#include "paint-funcs/mypaint-brushmodes.hpp"
#include "gimpmypaintcore-surface.hpp"
#include "base/delegators.hpp"

const float ALPHA_THRESHOLD = (float)((1 << 16) - 1);

GimpMypaintSurface::GimpMypaintSurface(GimpDrawable* d) 
  : undo_tiles(NULL), undo_desc(""), session(0), drawable(d), brush(NULL)
{
  g_object_add_weak_pointer(G_OBJECT(d), (gpointer*)&drawable);
}

GimpMypaintSurface::~GimpMypaintSurface()
{
  if (drawable)
    g_object_remove_weak_pointer(G_OBJECT(drawable), (gpointer*)&drawable);
  if (undo_tiles) {
    tile_manager_unref (undo_tiles);
    undo_tiles = NULL;
  }
  if (brush)
    g_object_unref(G_OBJECT(brush));
}

void 
GimpMypaintSurface::render_dab_mask_in_tile (Pixel::real *dab_mask,
                                             gint          *offsets,
                                             float x, float y,
                                             float radius,
                                             float hardness,
                                             float aspect_ratio, float angle,
                                             PixelRegion* srcPR,
                                             PixelRegion* channelPR
                                            ) {
  gint stride = srcPR->rowstride / srcPR->bytes;
//  g_print("dab_in_tile(x=%f,y=%f,r=%f,hd=%f,ar=%f,ang=%f,w=%d,h=%d,B=%d,s=%d)\n",x,y,radius,hardness,aspect_ratio,angle,w,h,bytes,stride);
  hardness = CLAMP(hardness, 0.0, 1.0);
  if (aspect_ratio<1.0) aspect_ratio=1.0;
    assert(hardness != 0.0); // assured by caller

  float r_fringe;
  int xp, yp;
  float xx, yy, rr;
  float one_over_radius2;
  Pixel::data_t* channel_data = NULL;

  r_fringe = radius + 1;
  rr = radius*radius;
  one_over_radius2 = 1.0/rr;

  // For a graphical explanation, see:
  // http://wiki.mypaint.info/Development/Documentation/Brushlib
  //
  // The hardness calculation is explained below:
  //
  // Dab opacity gradually fades out from the center (rr=0) to
  // fringe (rr=1) of the dab. How exactly depends on the hardness.
  // We use two linear segments, for which we pre-calculate slope
  // and offset here.
  //
  // opa
  // ^
  // *   .
  // |        *
  // |          .
  // +-----------*> rr = (distance_from_center/radius)^2
  // 0           1
  //
  float segment1_offset = 1.0;
  float segment1_slope  = -(1.0/hardness - 1.0);
  float segment2_offset = hardness/(1.0-hardness);
  float segment2_slope  = -hardness/(1.0-hardness);
  // for hardness == 1.0, segment2 will never be used

  float angle_rad=angle/360*2*M_PI;
  float cs=cos(angle_rad);
  float sn=sin(angle_rad);

  int x0 = floor (x - r_fringe);
  int y0 = floor (y - r_fringe);
  int x1 = ceil (x + r_fringe);
  int y1 = ceil (y + r_fringe);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > srcPR->w-1) x1 = srcPR->w-1;
  if (y1 > srcPR->h-1) y1 = srcPR->h-1;
  
//  g_print("dab area: (%d,%d)-(%d,%d)\n",x0,y0,x1,y1);
  
  // we do run length encoding: if opacity is zero, the next
  // value in the mask is the number of pixels that can be skipped.
  Pixel::real * dab_mask_p = dab_mask;
  int skip=0;
  
  if (channelPR) {
    channel_data = channelPR->data;
    channel_data += channelPR->rowstride * y0;
  }
  skip += y0*stride;
  for (yp = y0; yp <= y1; yp++) {
    yy = (yp + 0.5 - y);
    skip += x0;
    for (xp = x0; xp <= x1; xp++) {
      xx = (xp + 0.5 - x);
      // code duplication, see brush::count_dabs_to()
      float yyr=(yy*cs-xx*sn)*aspect_ratio;
      float xxr=yy*sn+xx*cs;
      rr = (yyr*yyr + xxr*xxr) * one_over_radius2;
      // rr is in range 0.0..1.0*sqrt(2)
      
      float opa;
      if (rr <= 1.0) {
        float fac;
        if (rr <= hardness) {
          opa = segment1_offset;
          fac = segment1_slope;
        } else {
          opa = segment2_offset;
          fac = segment2_slope;
        }
        opa += rr*fac;
        
#ifdef HEAVY_DEBUG
        assert(isfinite(opa));
        assert(opa >= 0.0 && opa <= 1.0);
#endif
      } else {
        opa = 0.0;
      }

      result_t opa_ = eval(pix(0.0f));
      if (channel_data)
        opa_ = eval( pix(opa) * pix(channel_data[xp]) );
      else
        opa_ = eval( pix(opa) );

      if (opa_ * ALPHA_THRESHOLD < 1.0) {
        skip++;
      } else {
        if (skip) {
          *dab_mask_p++ = 0;
          *offsets++ = skip;
          skip = 0;
        }
        *dab_mask_p++ = opa_;
      }
    }
    skip += stride-xp;
    if (channelPR)
      channel_data += channelPR->rowstride;
  }
  *dab_mask_p++ = 0;
  *offsets  = 0;
}

bool
GimpMypaintSurface::draw_dab (float x, float y, 
                              float radius, 
                              float color_r, float color_g, float color_b,
                              float opaque, float hardness,
                              float color_a,
                              float aspect_ratio, float angle,
                              float lock_alpha,float colorize)
{
  if (brush)
    return draw_brushmark_dab(x, y, radius, color_r, color_g, color_b, opaque,
                              hardness, color_a, aspect_ratio, angle, lock_alpha,
                              colorize);
  else
    return draw_mypaint_dab  (x, y, radius, color_r, color_g, color_b, opaque,
                              hardness, color_a, aspect_ratio, angle, lock_alpha,
                              colorize);
}

bool 
GimpMypaintSurface::draw_mypaint_dab (float x, float y, 
                         float radius, 
                         float color_r, float color_g, float color_b,
                         float opaque, float hardness,
                         float color_a,
                         float aspect_ratio, float angle,
                         float lock_alpha,float colorize
                         )
{
  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  gint            offset_x, offset_y;
  PixelRegion     src1PR;
  PixelRegion     maskPR;
  
//  g_print("Entering GimpMypaintSurface::draw_dab(x=%f,y=%f,r=%f,R=%f,G=%f,B=%f,o=%f,h=%f,a=%f,%f,%f,%f,%f)\n",x,y,radius,color_r,color_g,color_b,opaque,hardness,color_a,aspect_ratio,angle,lock_alpha,colorize);

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  opaque = CLAMP(opaque, 0.0, 1.0);
  hardness = CLAMP(hardness, 0.0, 1.0);
  lock_alpha = CLAMP(lock_alpha, 0.0, 1.0);
  colorize = CLAMP(colorize, 0.0, 1.0);
  if (radius < 0.1) return false; // don't bother with dabs smaller than 0.1 pixel
  if (hardness == 0.0) return false; // infintly small center point, fully transparent outside
  if (opaque == 0.0) return false;
//  assert(atomic > 0);
  // blending mode preparation
  float normal = 1.0;

  normal *= 1.0-lock_alpha;
  normal *= 1.0-colorize;

  if (aspect_ratio<1.0) aspect_ratio=1.0;

  float r_fringe = radius + 1;
  
  /* configure the pixel regions */
  int x1  = floor(x - r_fringe);
  int y1  = floor(y - r_fringe);
  int x2  = floor(x + r_fringe);
  int y2  = floor(y + r_fringe);

  int rx1 = x1;
  int ry1 = y1;
  int rx2 = x2;
  int ry2 = y2;

  GimpItem *mask_item = NULL;
  if (mask && !gimp_channel_is_empty(GIMP_CHANNEL(mask))) {
    mask_item = GIMP_ITEM (mask);

    /*  make sure coordinates are in mask bounds ...
     *  we need to add the layer offset to transform coords
     *  into the mask coordinate system
     */
    rx1 = CLAMP (rx1, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry1 = CLAMP (ry1, -offset_y, gimp_item_get_height (mask_item) - offset_y);
    rx2 = CLAMP (rx2, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry2 = CLAMP (ry2, -offset_y, gimp_item_get_height (mask_item) - offset_y);
  }
  rx1 = CLAMP (rx1, 0, gimp_item_get_width  (item) - 1);
  ry1 = CLAMP (ry1, 0, gimp_item_get_height (item) - 1);
  rx2 = CLAMP (rx2, 0, gimp_item_get_width  (item) - 1);
  ry2 = CLAMP (ry2, 0, gimp_item_get_height (item) - 1);

  int width   = (rx2 - rx1 + 1);
  int height   = (ry2 - ry1 + 1);

  if (rx1 > rx2 || ry1 > ry2)
    return false;

  /*  set undo blocks  */
  start_undo_group();
  validate_undo_tiles (rx1, ry1, width, height);

  pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                     rx1, ry1, width, height,
                     TRUE);

  if (mask_item) {
    pixel_region_init (&maskPR,
                       gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                       rx1 + offset_x,
                       ry1 + offset_y,
                       rx2 - rx1, ry2 - ry1,
                       TRUE);
  }

  struct DrawDabClosure {
    GimpMypaintSurface* surface;
    float x, y;
    float radius;
    float hardness;
    float aspect_ratio;
    float angle;
    float normal;
    float lock_alpha;
    float opaque;
    float color_a;
    Pixel::real opacity;
    Pixel::real rgba[4];
    Pixel::real bg_color[3];
    
    static void render_full(DrawDabClosure* closure, PixelRegion* src1PR, PixelRegion* maskPR) {
      // first, we calculate the mask (opacity for each pixel)
      Pixel::real dab_mask[TILE_WIDTH*TILE_HEIGHT+2*TILE_HEIGHT];
      gint dab_offsets[(TILE_HEIGHT + 2)*2];

      guchar  *s1 = src1PR->data;
      
      closure->surface->render_dab_mask_in_tile(dab_mask,
                      dab_offsets,
                      closure->x - src1PR->x,
                      closure->y - src1PR->y,
                      closure->radius,
                      closure->hardness,
                      closure->aspect_ratio, 
                      closure->angle,
                      src1PR, maskPR
                      );

      BrushPixelIteratorForRunLength iter(dab_mask, dab_offsets, s1, src1PR->bytes);
      // second, we use the mask to stamp a dab for each activated blend mode
      if (closure->normal) {
        if (closure->color_a == 1.0) {
          draw_dab_pixels_BlendMode_Normal(iter, 
                                           closure->rgba[0], closure->rgba[1], closure->rgba[2], 
                                           closure->normal * closure->opaque);
        } else {
          // normal case for brushes that use smudging (eg. watercolor)
          draw_dab_pixels_BlendMode_Normal_and_Eraser(iter,
                                                      closure->rgba[0], closure->rgba[1], closure->rgba[2], closure->rgba[3], 
                                                      closure->normal * closure->opaque, 
                                                      closure->bg_color[0], closure->bg_color[1], closure->bg_color[2]);
        }
      }

      if (closure->lock_alpha) {
        draw_dab_pixels_BlendMode_LockAlpha(iter,
                                            closure->rgba[0], closure->rgba[1], closure->rgba[2], 
                                            closure->lock_alpha * closure->opaque);
      }

  #if 0
      if (colorize) {
        draw_dab_pixels_BlendMode_Color(mask, rgba_p,
                                        color_r_, color_g_, color_b_,
                                        colorize*opaque*(1<<15));
      }
  #endif
          
    }
        
    static void render (DrawDabClosure* closure, PixelRegion* src1PR) {
      return render_full(closure, src1PR, NULL);
    }
  };
    
  DrawDabClosure closure;
  closure.surface = this;
  closure.x = x;
  closure.y = y;
  closure.radius = radius;
  closure.hardness = hardness;
  closure.aspect_ratio = aspect_ratio;
  closure.angle = angle;
  closure.normal = normal;
  closure.color_a = color_a;
  closure.normal = normal;
  closure.opaque = opaque;
  closure.lock_alpha = lock_alpha;
  closure.rgba[0] = color_r;
  closure.rgba[1] = color_g;
  closure.rgba[2] = color_b;
  closure.rgba[3] = color_a;
  closure.bg_color[0] = bg_color.r;
  closure.bg_color[1] = bg_color.g;
  closure.bg_color[2] = bg_color.b;

  if (!mask_item)
    pixel_regions_process_parallel((PixelProcessorFunc)DrawDabClosure::render, &closure, 1, &src1PR);
  else
    pixel_regions_process_parallel((PixelProcessorFunc)DrawDabClosure::render_full, &closure, 2, &src1PR, &maskPR);
  
  
  /*  Update the drawable  */
  gimp_drawable_update (drawable, rx1, ry1, width, height);
  if (rx1 < this->x1)
    this->x1 = rx1;
  if (ry1 < this->y1)
    this->y1 = ry1;
  if (rx1 + width > this->x2)
    this->x2 = rx1 + width;
  if (ry1 + height > this->y2)
    this->y2 = ry1 + height;

  return true;
}


bool 
GimpMypaintSurface::draw_brushmark_dab (float x, float y, 
                                        float radius, 
                                        float color_r, float color_g, float color_b,
                                        float opaque, float hardness,
                                        float color_a,
                                        float aspect_ratio, float angle,
                                        float lock_alpha,float colorize)
{
  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  gint            offset_x, offset_y;
  PixelRegion     src1PR, brushPR;
  PixelRegion     maskPR;
  
  int brush_radius = MAX(brush->mask->width, brush->mask->height);
  if (brush_radius < 1) return false;
  float scale = radius * 2 / brush_radius;

  opaque = CLAMP(opaque, 0.0, 1.0);
  hardness = CLAMP(hardness, 0.0, 1.0);
  lock_alpha = CLAMP(lock_alpha, 0.0, 1.0);
  colorize = CLAMP(colorize, 0.0, 1.0);
  if (radius < 0.1) return false; // don't bother with dabs smaller than 0.1 pixel
  if (hardness == 0.0) return false; // infintly small center point, fully transparent outside
  if (opaque == 0.0) return false;

  if (aspect_ratio<1.0) aspect_ratio=1.0;

#if 0
  ScopeGuard<TempBuf, void(TempBuf*)> dab_mask(
      (TempBuf*)gimp_brush_transform_mask (brush,
                                 scale,
                                 aspect_ratio,
                                 angle,
                                 hardness),
      temp_buf_free);
#else
  float gimp_brush_aspect_ratio = 20 * (1.0 - ( 1.0 / aspect_ratio));
  const TempBuf* dab_mask=
    gimp_brush_transform_mask (brush,
                               scale,
                               gimp_brush_aspect_ratio,
                               -angle / 360,
                               hardness);
#endif

//  g_print("scale,aspect,angle,hardness=%f,%f->%f,%f,%f\n", scale, aspect_ratio, gimp_brush_aspect_ratio, angle, hardness);

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  // blending mode preparation
  float normal = 1.0;

  normal *= 1.0-lock_alpha;
  normal *= 1.0-colorize;

  int radius_x  = (dab_mask->width + 1) / 2;
  int radius_y  = (dab_mask->height + 1) / 2;
  /* configure the pixel regions */
  int x1  = floor(x - radius_x);
  int y1  = floor(y - radius_y);
  int x2  = floor(x1 + dab_mask->width - 1);
  int y2  = floor(y1 + dab_mask->height - 1);

  int rx1 = x1;
  int ry1 = y1;
  int rx2 = x2;
  int ry2 = y2;
  
  GimpItem *mask_item = NULL;
  if (mask && !gimp_channel_is_empty(GIMP_CHANNEL(mask))) {
    mask_item = GIMP_ITEM (mask);

    /*  make sure coordinates are in mask bounds ...
     *  we need to add the layer offset to transform coords
     *  into the mask coordinate system
     */
    rx1 = CLAMP (rx1, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry1 = CLAMP (ry1, -offset_y, gimp_item_get_height (mask_item) - offset_y);
    rx2 = CLAMP (rx2, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry2 = CLAMP (ry2, -offset_y, gimp_item_get_height (mask_item) - offset_y);
  }
  rx1 = CLAMP (rx1, 0, gimp_item_get_width  (item) - 1);
  ry1 = CLAMP (ry1, 0, gimp_item_get_height (item) - 1);
  rx2 = CLAMP (rx2, 0, gimp_item_get_width  (item) - 1);
  ry2 = CLAMP (ry2, 0, gimp_item_get_height (item) - 1);

  int width   = (rx2 - rx1 + 1);
  int height   = (ry2 - ry1 + 1);

  if (rx1 > rx2 || ry1 > ry2)
    return false;

  /*  set undo blocks  */
  start_undo_group();
  validate_undo_tiles (rx1, ry1, width, height);

  pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                     rx1, ry1, width, height,
                     TRUE);

  pixel_region_init_temp_buf(&brushPR, (TempBuf*)dab_mask, MAX(rx1 - x1, 0), MAX(ry1 - y1, 0), width, height);

  if (mask_item) {
    pixel_region_init (&maskPR,
                       gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                       rx1 + offset_x,
                       ry1 + offset_y,
                       rx2 - rx1, ry2 - ry1,
                       TRUE);
  }

  struct DrawBrushmarkClosure {
    GimpMypaintSurface* surface;
    float x, y;
    float radius;
    float hardness;
    float aspect_ratio;
    float angle;
    float normal;
    float lock_alpha;
    float opaque;
    float color_a;
    Pixel::real opacity;
    Pixel::real rgba[4];
    Pixel::real bg_color[3];
    
    static void render_full(DrawBrushmarkClosure* closure, PixelRegion* src1PR, PixelRegion* brushPR, PixelRegion* maskPR) {

      guchar* rgba     = src1PR->data;
      guchar* dab_mask = brushPR->data;

      BrushPixelIteratorForBrushmark iter(dab_mask, rgba, 
                                          src1PR->w, src1PR->h, 
                                          brushPR->rowstride, src1PR->rowstride, 
                                          src1PR->bytes);
      // second, we use the mask to stamp a dab for each activated blend mode
      if (closure->normal) {
        if (closure->color_a == 1.0) {
          draw_dab_pixels_BlendMode_Normal(iter, 
                                           closure->rgba[0], closure->rgba[1], closure->rgba[2], 
                                           closure->normal * closure->opaque);
        } else {
          // normal case for brushes that use smudging (eg. watercolor)
          draw_dab_pixels_BlendMode_Normal_and_Eraser(iter,
                                                      closure->rgba[0], closure->rgba[1], closure->rgba[2], closure->rgba[3], 
                                                      closure->normal * closure->opaque, 
                                                      closure->bg_color[0], closure->bg_color[1], closure->bg_color[2]);
        }
      }

      if (closure->lock_alpha) {
        draw_dab_pixels_BlendMode_LockAlpha(iter,
                                            closure->rgba[0], closure->rgba[1], closure->rgba[2], 
                                            closure->lock_alpha * closure->opaque);
      }
          
    }
        
    static void render (DrawBrushmarkClosure* closure, PixelRegion* src1PR, PixelRegion* brushPR) {
      return render_full(closure, src1PR, brushPR, NULL);
    }
  };
    
  DrawBrushmarkClosure closure;
  closure.surface = this;
  closure.x = x;
  closure.y = y;
  closure.radius = radius;
  closure.hardness = hardness;
  closure.aspect_ratio = aspect_ratio;
  closure.angle = angle;
  closure.normal = normal;
  closure.color_a = color_a;
  closure.normal = normal;
  closure.opaque = opaque;
  closure.lock_alpha = lock_alpha;
  closure.rgba[0] = color_r;
  closure.rgba[1] = color_g;
  closure.rgba[2] = color_b;
  closure.rgba[3] = color_a;
  closure.bg_color[0] = bg_color.r;
  closure.bg_color[1] = bg_color.g;
  closure.bg_color[2] = bg_color.b;

  if (!mask_item)
    pixel_regions_process_parallel((PixelProcessorFunc)DrawBrushmarkClosure::render, &closure, 2, &src1PR, &brushPR);
  else
    pixel_regions_process_parallel((PixelProcessorFunc)DrawBrushmarkClosure::render_full, &closure, 3, &src1PR, &brushPR, &maskPR);
  
  
  /*  Update the drawable  */
  gimp_drawable_update (drawable, rx1, ry1, width, height);
  if (rx1 < this->x1)
    this->x1 = rx1;
  if (ry1 < this->y1)
    this->y1 = ry1;
  if (rx1 + width > this->x2)
    this->x2 = rx1 + width;
  if (ry1 + height > this->y2)
    this->y2 = ry1 + height;

  return true;
}

void 
GimpMypaintSurface::get_color (float x, float y, 
                          float radius, 
                          float * color_r, float * color_g, float * color_b, float * color_a
                          )
{
  if (radius < 1.0) radius = 1.0;

  float sum_weight, sum_r, sum_g, sum_b, sum_a;
  sum_weight = sum_r = sum_g = sum_b = sum_a = 0.0;
  const float hardness = 0.5;
  const float aspect_ratio = 1.0;
  const float angle = 0.0;

  // in case we return with an error
  *color_r = 0.0;
  *color_g = 1.0;
  *color_b = 0.0;

  // WARNING: some code duplication with draw_dab

  GimpItem        *item  = GIMP_ITEM (drawable);
  GimpImage       *image = gimp_item_get_image (item);
  GimpChannel     *mask  = gimp_image_get_mask (image);
  gint             offset_x, offset_y;
  PixelRegion      src1PR;
  PixelRegion maskPR;
  
  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  float r_fringe = radius + 1;
  
  /* configure the pixel regions */
  int x1  = floor(x - r_fringe);
  int y1  = floor(y - r_fringe);
  int x2  = floor(x + r_fringe);
  int y2  = floor(y + r_fringe);

  int rx1 = x1;
  int ry1 = y1;
  int rx2 = x2;
  int ry2 = y2;

  if (rx1 > rx2 || ry1 > ry2)
    return;

  GimpItem *mask_item = NULL;
  if (mask && !gimp_channel_is_empty(GIMP_CHANNEL(mask))) {
    mask_item = GIMP_ITEM (mask);

    /*  make sure coordinates are in mask bounds ...
     *  we need to add the layer offset to transform coords
     *  into the mask coordinate system
     */
    rx1 = CLAMP (rx1, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry1 = CLAMP (ry1, -offset_y, gimp_item_get_height (mask_item) - offset_y);
    rx2 = CLAMP (rx2, -offset_x, gimp_item_get_width  (mask_item) - offset_x);
    ry2 = CLAMP (ry2, -offset_y, gimp_item_get_height (mask_item) - offset_y);
  }
  rx1 = CLAMP (rx1, 0, gimp_item_get_width  (item) - 1);
  ry1 = CLAMP (ry1, 0, gimp_item_get_height (item) - 1);
  rx2 = CLAMP (rx2, 0, gimp_item_get_width  (item) - 1);
  ry2 = CLAMP (ry2, 0, gimp_item_get_height (item) - 1);
  
  int width   = (rx2 - rx1 + 1);
  int height   = (ry2 - ry1 + 1);

  pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                     rx1, ry1, width, height,
                     FALSE);

  gpointer pr;
  if (mask_item) {
    pixel_region_init (&maskPR,
                       gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                       rx1 + offset_x,
                       ry1 + offset_y,
                       rx2 - rx1, ry2 - ry1,
                       FALSE);

    pr = pixel_regions_register (2, &src1PR, &maskPR);
  } else {
    pr = pixel_regions_register (1, &src1PR);
  }

  // first, we calculate the mask (opacity for each pixel)
  static Pixel::real dab_mask[TILE_WIDTH*TILE_HEIGHT+2*TILE_HEIGHT];
  static gint dab_offsets[(TILE_HEIGHT + 2)*2];
  for (;
       pr != NULL;
       pr = pixel_regions_process ((PixelRegionIterator*)pr)) {
    guchar  *s1 = src1PR.data;

    render_dab_mask_in_tile(dab_mask,
                    dab_offsets,
                    x - src1PR.x,
                    y - src1PR.y,
                    radius,
                    hardness,
                    aspect_ratio, angle,
                    &src1PR, (mask_item)? &maskPR : NULL
                    );

    BrushPixelIteratorForRunLength iter(dab_mask, dab_offsets, s1, src1PR.bytes);

    get_color_pixels_accumulate (iter,
                                 &sum_weight, &sum_r, &sum_g, &sum_b, &sum_a);
  }

//  assert(sum_weight > 0.0);
  if (sum_weight > 0.0) {
    sum_a /= sum_weight;
    sum_r /= sum_weight;
    sum_g /= sum_weight;
    sum_b /= sum_weight;
  }

  *color_a = sum_a;
  // now un-premultiply the alpha
  if (sum_a > 0.0) {
    *color_r = sum_r / sum_a;
    *color_g = sum_g / sum_a;
    *color_b = sum_b / sum_a;
  } else {
    // it is all transparent, so don't care about the colors
    // (let's make them ugly so bugs will be visible)
    *color_r = 0.0;
    *color_g = 1.0;
    *color_b = 0.0;
  }

  // fix rounding problems that do happen due to floating point math
  *color_r = CLAMP(*color_r, 0.0, 1.0);
  *color_g = CLAMP(*color_g, 0.0, 1.0);
  *color_b = CLAMP(*color_b, 0.0, 1.0);
  *color_a = CLAMP(*color_a, 0.0, 1.0);
}

void 
GimpMypaintSurface::begin_session()
{
  session = 0;
}

void 
GimpMypaintSurface::end_session()
{
  if (session <= 0)
    return;
    
  stop_updo_group();
  session = 0;
}

void 
GimpMypaintSurface::start_undo_group()
{
  GimpItem *item;
  
  if (session > 0)
    return;
  g_print("Stroke::start_undo_group...\n");
  session = 1;

  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_print("Stroke::begin_session:: do begining of session...\n");
//  g_return_if_fail (coords != NULL);
//  g_return_if_fail (error == NULL || *error == NULL);

  item = GIMP_ITEM (drawable);

  /*  Allocate the undo structure  */
  if (undo_tiles)
    tile_manager_unref (undo_tiles);

  undo_tiles = tile_manager_new (gimp_item_get_width  (item),
                                 gimp_item_get_height (item),
                                 gimp_drawable_bytes (drawable));

  /*  Get the initial undo extents  */
  x1 = gimp_item_get_width (item) + 1;
  y1 = gimp_item_get_height (item) + 1;
  x2 = -1;
  y2 = -1;

  /*  Freeze the drawable preview so that it isn't constantly updated.  */
  gimp_viewable_preview_freeze (GIMP_VIEWABLE (drawable));

  return;
}

void 
GimpMypaintSurface::stop_updo_group()
{
  GimpImage *image;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  /*  Determine if any part of the image has been altered--
   *  if nothing has, then just return...
   */
  if (x2 < x1 || y2 < y1) {
    gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
    return;
  }

  g_print("Stroke::end_session::push_undo(%d,%d)-(%d,%d)\n",x1,y1,x2,y2);
  push_undo (image, "Mypaint Brush");

  gimp_viewable_preview_thaw (GIMP_VIEWABLE (drawable));
  
  return;
}

GimpUndo *
GimpMypaintSurface::push_undo (GimpImage     *image,
                                const gchar   *undo_desc)
{
  gimp_image_undo_group_start (image, GIMP_UNDO_GROUP_MYPAINT,
                               undo_desc);

  gimp_image_undo_push (image, GIMP_TYPE_MYPAINT_CORE_UNDO,
                               GIMP_UNDO_PAINT, NULL,
                               GimpDirtyMask(0),
                               NULL);
  if (undo_tiles) {
    gimp_image_undo_push_drawable (image, "Mypaint Brush",
                                   drawable, undo_tiles,
                                   TRUE, x1, y1, x2 - x1, y2 - y1);
  }
  gimp_image_undo_group_end (image);

  tile_manager_unref (undo_tiles);
  undo_tiles = NULL;

  return NULL;
}

#if 0
// copy unmodified tiles from undo tile stack
void
GimpMypaintSurface::copy_valid_tiles (TileManager *src_tiles,
                                  TileManager *dest_tiles,
                                  gint         x,
                                  gint         y,
                                  gint         w,
                                  gint         h)
{
  Tile *src_tile;
  gint  i, j;

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT))) {
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH))) {
      src_tile = tile_manager_get_tile (src_tiles, j, i, FALSE, FALSE);

      if (tile_is_valid (src_tile)) {
        src_tile = tile_manager_get_tile (src_tiles, j, i, TRUE, FALSE);
        tile_manager_map_tile (dest_tiles, j, i, src_tile);
        tile_release (src_tile, FALSE);
      }
    }
  }

}
#endif
#if 0
void
GimpMypaintSurface::paste (PixelRegion              *mypaint_maskPR,
                       GimpDrawable             *drawable,
                       gdouble                   mypaint_opacity,
                       gdouble                   image_opacity,
                       GimpLayerModeEffects      mypaint_mode,
                       GimpMypaintBrushMode  mode)
{
  TileManager *alt = NULL;
  PixelRegion  srcPR;

  /*  set undo blocks  */
  gimp_mypaint_core_validate_undo_tiles (core, drawable,
                                       core->canvas_buf->x,
                                       core->canvas_buf->y,
                                       core->canvas_buf->width,
                                       core->canvas_buf->height);

  mypaint_mask_to_canvas_buf (core, mypaint_maskPR, mypaint_opacity);

  /*  intialize canvas buf source pixel regions  */
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

  /*  apply the mypaint area to the image  */
  gimp_drawable_apply_region (drawable, &srcPR,
                              FALSE, NULL,
                              image_opacity, mypaint_mode,
                              alt,  /*  specify an alternative src1  */
                              NULL,
                              core->canvas_buf->x,
                              core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width);
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height);

  /*  Update the drawable  */
  gimp_drawable_update (drawable,
                        core->canvas_buf->x,
                        core->canvas_buf->y,
                        core->canvas_buf->width,
                        core->canvas_buf->height);

}
#endif

/* This works similarly to gimp_mypaint_core_paste. However, instead of
 * combining the canvas to the mypaint core drawable using one of the
 * combination modes, it uses a "replace" mode (i.e. transparent
 * pixels in the canvas erase the mypaint core drawable).

 * When not drawing on alpha-enabled images, it just mypaints using
 * NORMAL mode.
 */
#if 0
void
GimpMypaintSurface::replace (PixelRegion              *mypaint_maskPR,
                         GimpDrawable             *drawable,
                         gdouble                   mypaint_opacity,
                         gdouble                   image_opacity,
                         GimpMypaintBrushMode  mode)
{
  PixelRegion  srcPR;

  if (! gimp_drawable_has_alpha (drawable))
    {
      gimp_mypaint_core_paste (core, mypaint_maskPR, drawable,
                             mypaint_opacity,
                             image_opacity, GIMP_NORMAL_MODE,
                             mode);
      return;
    }

  /*  set undo blocks  */
  validate_undo_tiles (drawable,
                       canvas_buf->x,
                       canvas_buf->y,
                       canvas_buf->width,
                       canvas_buf->height);

  /*  intialize canvas buf source pixel regions  */
  pixel_region_init_temp_buf (&srcPR, core->canvas_buf,
                              0, 0,
                              core->canvas_buf->width,
                              core->canvas_buf->height);

  /*  apply the mypaint area to the image  */
  gimp_drawable_replace_region (drawable, &srcPR,
                                FALSE, NULL,
                                image_opacity,
                                mypaint_maskPR,
                                core->canvas_buf->x,
                                core->canvas_buf->y);

  /*  Update the undo extents  */
  core->x1 = MIN (core->x1, core->canvas_buf->x);
  core->y1 = MIN (core->y1, core->canvas_buf->y);
  core->x2 = MAX (core->x2, core->canvas_buf->x + core->canvas_buf->width) ;
  core->y2 = MAX (core->y2, core->canvas_buf->y + core->canvas_buf->height) ;

  /*  Update the drawable  */
  gimp_drawable_update (drawable,
                        core->canvas_buf->x,
                        core->canvas_buf->y,
                        core->canvas_buf->width,
                        core->canvas_buf->height);
}
#endif

void
GimpMypaintSurface::validate_undo_tiles (
                                     gint           x,
                                     gint           y,
                                     gint           w,
                                     gint           h)
{
  gint i, j;
  GimpItem* item = GIMP_ITEM (drawable);

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (undo_tiles != NULL);
  
  if (x + w > gimp_item_get_width(item))
    w = MAX(0, gimp_item_get_width(item) - x);
  if (y + h > gimp_item_get_height(item))
    h = MAX(0, gimp_item_get_height(item) - y);
  
  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT))) {
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH))) {
      Tile *dest_tile = tile_manager_get_tile (undo_tiles, j, i, FALSE, FALSE);
      assert(j< gimp_item_get_width(item));
      assert(i< gimp_item_get_height(item));

      if (! tile_is_valid (dest_tile)) {
        Tile *src_tile =
          tile_manager_get_tile (gimp_drawable_get_tiles (drawable),
                                 j, i, TRUE, FALSE);
        tile_manager_map_tile (undo_tiles, j, i, src_tile);
        tile_release (src_tile, FALSE);
//        g_print("validate_undo_tiles:backup xy=(%d,%d),bound=(%d,%d)\n",j,i,MIN(x+w-j, TILE_WIDTH), MIN(y+h-i,TILE_HEIGHT));

      }
    }
  }
  
}

void 
GimpMypaintSurface::set_bg_color (GimpRGB* src) 
{ 
  if (src)
    bg_color = *src; 
}

void 
GimpMypaintSurface::get_bg_color (GimpRGB* dest) 
{
  if (dest)
    *dest = bg_color; 
}

void 
GimpMypaintSurface::set_brush(GimpBrush* brush_)
{
  if (brush) {
    gimp_brush_end_use(brush);
    g_object_unref(G_OBJECT(brush));
    brush = NULL;
  }

  if (brush_) {
    brush = brush_;
    gimp_brush_begin_use(brush);
    g_object_ref(G_OBJECT(brush));
  }
}

GimpBrush* 
GimpMypaintSurface::get_brush()
{
  return brush;
}
