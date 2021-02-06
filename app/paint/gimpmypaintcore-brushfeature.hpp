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

#ifndef __GIMPMYPAINTCORE_BRUSHFEATURE_HPP__
#define __GIMPMYPAINTCORE_BRUSHFEATURE_HPP__

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
#include "core/gimppattern.h"
#include "core/gimppickable.h"
#include "core/gimpprojection.h"

#include "gimpmypaintcore.hpp"
#include "gimpmypaintcoreundo.h"
#include "gimpmypaintoptions.h"

#include "gimp-intl.h"
#include "paint-funcs/paint-funcs.h"
};

#define REAL_CALC

#include "base/pixel.hpp"
#include "base/scopeguard.hpp"
#include "paint-funcs/mypaint-brushmodes.hpp"
#include "gimpmypaintcore-surface.hpp"
#include "base/delegators.hpp"
#include "base/glib-cxx-utils.hpp"

const float ALPHA_THRESHOLD = (float)((1 << 16) - 1);

////////////////////////////////////////////////////////////////////////////////
template<typename PixelIter>
class GeneralBrushFeature {
public:
  class ColorAccumulator {
#ifdef ENABLE_MP
    GMutex              mutex;
#endif
    float sum_weight, sum_r, sum_g, sum_b, sum_a;

  public:
  ColorAccumulator() {
    reset();
#ifdef ENABLE_MP
    g_mutex_init(&mutex);
#endif
    };
  
    ~ColorAccumulator() {
#ifdef ENABLE_MP
      g_mutex_clear(&mutex);
#endif
    }

    void reset() {
      sum_weight = sum_r = sum_g = sum_b = sum_a = 0.0;
    }
  
    void accumulate(float w, float r, float g, float b, float a) {
#ifdef ENABLE_MP
      g_mutex_lock (&mutex);
#endif
      sum_weight += w;
      sum_r += r;
      sum_g += g;
      sum_b += b;
      sum_a += a;
#ifdef ENABLE_MP
      g_mutex_unlock (&mutex);
#endif
    }

    void summarize(float* r, float* g, float* b, float* a) {
      if (sum_weight > 0.0) {
        sum_r /= sum_weight;
        sum_g /= sum_weight;
        sum_b /= sum_weight;
        sum_a /= sum_weight;
      }
      if (sum_a > 0.0) {
        sum_r /= sum_a;
        sum_g /= sum_a;
        sum_b /= sum_a;
      } else {
        // it is all transparent, so don't care about the colors
        // (let's make them ugly so bugs will be visible)
        sum_r = 0.0;
        sum_g = 1.0;
        sum_b = 0.0;
      }
      // fix rounding problems that do happen due to floating point math
      *r = CLAMP(sum_r, 0.0, 1.0);
      *g = CLAMP(sum_g, 0.0, 1.0);
      *b = CLAMP(sum_b, 0.0, 1.0);
      *a = CLAMP(sum_a, 0.0, 1.0);
      reset();
   }
  };
  
protected:
  float x, y;
  float opaque;
  float normal, lock_alpha;
  float stroke_opacity;
  float color_a;
  Pixel::real fg_color[4];
  Pixel::real bg_color[3];
  float texture_grain;
  float texture_contrast;
  ColorAccumulator accumulator;
  
public:
  typedef PixelIter iterator;
  bool 
  prepare_brush(float x, float y, float radius, 
                float hardness, float aspect_ratio, float angle, 
                float normal, float opaque, float lock_alpha, 
                Pixel::real* fg_color, float color_a, Pixel::real* bg_color, 
                float stroke_opacity, float texture_grain, float texture_contrast,
                void* /*unused*/)
  {
    this->x                = x;
    this->y                = y;
    this->normal           = normal;
    this->opaque           = opaque;
    this->lock_alpha       = lock_alpha;
    this->color_a          = color_a;
    this->fg_color[0]      = fg_color[0];
    this->fg_color[1]      = fg_color[1];
    this->fg_color[2]      = fg_color[2];
    this->fg_color[3]      = fg_color[3];
    this->bg_color[0]      = bg_color[0];
    this->bg_color[1]      = bg_color[1];
    this->bg_color[2]      = bg_color[2];
    this->stroke_opacity   = stroke_opacity;
    this->texture_grain    = texture_grain;
    this->texture_contrast = texture_contrast;
    return true;
  }


  void
  draw_dab(PixelIter& iter) 
  {
    // second, we use the mask to stamp a dab for each activated blend mode
    if (normal) {
      if (color_a == 1.0) {
        draw_dab_pixels_BlendMode_Normal(iter, 
                                         normal * opaque);
      } else {
        // normal case for brushes that use smudging (eg. watercolor)
        draw_dab_pixels_BlendMode_Normal_and_Eraser(iter,
                                                    color_a,
                                                    normal * opaque, 
                                                    bg_color[0], bg_color[1], bg_color[2]);
      }
    }

    if (lock_alpha) {
      draw_dab_pixels_BlendMode_LockAlpha(iter,
                                          lock_alpha * opaque);
    }
  }
  

  void
  copy_stroke(PixelRegion* src1PR, PixelRegion* destPR, 
              PixelRegion* brushPR, PixelRegion* maskPR,
              PixelRegion* texturePR) 
  {
    BrushPixelIteratorForPlainData<PixmapBrushmarkIterator, Pixel::data_t, Pixel::data_t>
       iter(brushPR->data, NULL, src1PR->data, destPR->data, src1PR->w, src1PR->h, 
            brushPR->rowstride, src1PR->rowstride, destPR->rowstride,
            brushPR->bytes, src1PR->bytes, destPR->bytes);
    
    // normal case for brushes that use smudging (eg. watercolor)
    draw_dab_pixels_BlendMode_Normal_and_Eraser(iter, 1.0, stroke_opacity, 
                                                bg_color[0], bg_color[1], bg_color[2]);
  }

  ColorAccumulator* get_accumulator() {
    return &accumulator;
  }
  
};

////////////////////////////////////////////////////////////////////////////////
class MypaintBrushFeature : 
  public GeneralBrushFeature<BrushPixelIteratorForRunLength>
{
  float radius;
  float hardness, aspect_ratio, angle;

public:
  typedef BrushPixelIteratorForRunLength iterator;
  typedef GeneralBrushFeature<iterator> Parent;

  bool 
  prepare_brush(float x, float y, float radius, 
                float hardness, float aspect_ratio, float angle, 
                float normal, float opaque, float lock_alpha, 
                Pixel::real* fg_color, float color_a, Pixel::real* bg_color, 
                float stroke_opacity, float texture_grain, float texture_contrast, 
                void* brush)
  {
    Parent::prepare_brush(x, y, radius, 
                          hardness, aspect_ratio, angle, 
                          normal, opaque, lock_alpha, 
                          fg_color, color_a, bg_color, stroke_opacity,
                          texture_grain, texture_contrast, brush);
    this->radius       = radius;
    this->hardness     = hardness;
    this->aspect_ratio = aspect_ratio;
    this->angle        = angle;
    return true;
  }
  
  TempBuf* get_brush_data() { return NULL; }
  

  void 
  get_boundary(int&x1, int& y1, int& x2, int& y2) 
  {
    float r_fringe = radius + 1;
    
    x1  = floor(x - r_fringe);
    y1  = floor(y - r_fringe);
    x2  = floor(x + r_fringe);
    y2  = floor(y + r_fringe);
  };

  
  void 
  fill_brushmark_buffer (Pixel::real *dab_mask,
                         gint          *offsets,
                         float cx_in_tile_coords, 
                         float cy_in_tile_coords,
                         PixelRegion* srcPR,
                         PixelRegion* channelPR,
                         PixelRegion* texturePR) 
  {
    g_return_if_fail(srcPR->bytes != 0);
    gint stride = srcPR->rowstride / srcPR->bytes;
    hardness = CLAMP(hardness, 0.0, 1.0);
    if (aspect_ratio<1.0) aspect_ratio=1.0;
      assert(hardness != 0.0); // assured by caller

    float r_fringe;
    int xp, yp;
    float xx, yy, rr;
    float one_over_radius2;
    Pixel::data_t* channel_data = NULL;
    Pixel::data_t* texture_data = NULL;

    r_fringe = radius + 1;
    rr = radius*radius;
    g_return_if_fail (rr > 0);
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
    float segment2_offset = (hardness != 1.0) ? hardness/(1.0-hardness): 0;
    float segment2_slope  = (hardness != 1.0) ? -hardness/(1.0-hardness): 0;
    // for hardness == 1.0, segment2 will never be used

    float angle_rad=angle/360*2*M_PI;
    float cs=cos(angle_rad);
    float sn=sin(angle_rad);

    int x0 = floor (cx_in_tile_coords - r_fringe);
    int y0 = floor (cy_in_tile_coords - r_fringe);
    int x1 = ceil (cx_in_tile_coords + r_fringe);
    int y1 = ceil (cy_in_tile_coords + r_fringe);
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > srcPR->w-1) x1 = srcPR->w-1;
    if (y1 > srcPR->h-1) y1 = srcPR->h-1;
    
//    g_print("dab area: (%d,%d)-(%d,%d)\n",x0,y0,x1,y1);
    
    // we do run length encoding: if opacity is zero, the next
    // value in the mask is the number of pixels that can be skipped.
    Pixel::real * dab_mask_p = dab_mask;
    int skip=0;
    
    if (channelPR) {
      channel_data = channelPR->data;
      channel_data += channelPR->rowstride * y0;
    }

    if (texturePR) {
      texture_data = texturePR->data;
      texture_data += texturePR->rowstride * y0;
    }
    
    skip += y0*stride;
    for (yp = y0; yp <= y1; yp++) {
      yy = (yp + 0.5 - cy_in_tile_coords);
      skip += x0;
      for (xp = x0; xp <= x1; xp++) {
        xx = (xp + 0.5 - cx_in_tile_coords);
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

        if (texture_data) {
          opa_ = eval( pix(opa_) * 
                      (pix(texture_data[xp * texturePR->bytes]) + pix(texture_grain)) * pix(texture_contrast));
        }

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
      if (texturePR)
        texture_data += texturePR->rowstride;
    }
    *dab_mask_p++ = 0;
    *offsets  = 0;
  }

  void 
  draw_dab(PixelRegion* src1PR, 
           PixelRegion* destPR,
           PixelRegion* , // unused 
           PixelRegion* maskPR,
           PixelRegion* texturePR)
  {
    // first, we calculate the mask (opacity for each pixel)
    Pixel::real dab_mask[MAX(TILE_WIDTH, src1PR->w)*MAX(TILE_HEIGHT, src1PR->h)+2];
    gint dab_offsets[MAX(TILE_WIDTH, src1PR->w) * MAX(TILE_HEIGHT, src1PR->h)];

    Pixel::data_t* src_data  = src1PR->data;
    Pixel::data_t* dest_data = destPR->data;

    fill_brushmark_buffer (dab_mask,
                           dab_offsets,
                           x - src1PR->x,
                           y - src1PR->y,
                           src1PR,
                           maskPR,
                           texturePR);

    iterator iter(dab_mask, 
                  dab_offsets, 
                  fg_color, 
                  src_data, 
                  dest_data, 
                  src1PR->bytes, 
                  destPR->bytes);
    
    Parent::draw_dab(iter);
  }

  void
  copy_stroke(PixelRegion* src1PR, PixelRegion* destPR, 
              PixelRegion* brushPR, PixelRegion* maskPR,
              PixelRegion* texturePR) 
  {
    Parent::copy_stroke(src1PR, destPR, brushPR, maskPR, texturePR);
  }

  void
  get_color(PixelRegion* src1PR, 
            PixelRegion* brushPR, 
            PixelRegion* maskPR,
            PixelRegion* texturePR)
  {
    float sum_weight, sum_r, sum_g, sum_b, sum_a;
    sum_weight = sum_r = sum_g = sum_b = sum_a = 0.0;

    Pixel::real dab_mask[(MAX(TILE_WIDTH, src1PR->w) + 2)*MAX(TILE_HEIGHT, src1PR->h)];
    gint dab_offsets[(MAX(TILE_HEIGHT, src1PR->h) + 2)*2];

    Pixel::data_t*  src_data = src1PR->data;
    
    iterator iter(dab_mask, dab_offsets, 
                  NULL, src_data, src_data,  
                  src1PR->bytes, src1PR->bytes);

    fill_brushmark_buffer(dab_mask, dab_offsets,
                          x - src1PR->x, y - src1PR->y,
                          src1PR, maskPR, NULL);

    get_color_pixels_accumulate (iter,
                                 &sum_weight, &sum_r, &sum_g, &sum_b, &sum_a);
    accumulator.accumulate(sum_weight, sum_r, sum_g, sum_b, sum_a);
  }
  
};

////////////////////////////////////////////////////////////////////////////////
class GimpBrushFeature : 
  public GeneralBrushFeature<BrushPixelIteratorForPlainData<ColoredBrushmarkIterator, Pixel::real, Pixel::real> >
{
  GimpCoords* last_coords;
  GimpCoords* current_coords;
  const TempBuf* dab_mask;
  float radius;
  
public:
  typedef BrushPixelIteratorForPlainData<ColoredBrushmarkIterator, Pixel::real, Pixel::real> iterator;
  typedef GeneralBrushFeature<iterator> Parent;
  GimpBrushFeature(GimpCoords* current_coords,
                                 GimpCoords* last_coords)
    : dab_mask(NULL)
  {
    this->current_coords = current_coords;
    this->last_coords    = last_coords;
  };
  ~GimpBrushFeature() {
  }

  const TempBuf* get_brush_data()
  { 
    return dab_mask;
  }


  bool 
  prepare_brush(float x, float y, float radius, 
                float hardness, float aspect_ratio, float angle, 
                float normal, float opaque, float lock_alpha, 
                Pixel::real* fg_color, float color_a, Pixel::real* bg_color, 
                float stroke_opacity,
                float texture_grain, float texture_contrast, void* data)
  {
    Parent::prepare_brush(x, y, radius, 
                          hardness, aspect_ratio, angle, 
                          normal, opaque, lock_alpha, 
                          fg_color, color_a, bg_color,stroke_opacity, 
                          texture_grain, texture_contrast, data);
    dab_mask         = NULL;
    GimpBrush* brush = GIMP_BRUSH(data);
    this->radius     = radius;
    int brush_radius = MAX(brush->mask->width, brush->mask->height);

    if (brush_radius < 1)  return false;

    float scale                   = radius * 2 / brush_radius;
    float gimp_brush_aspect_ratio = 20 * (1.0 - ( 1.0 / aspect_ratio));

    GimpBrush* current_brush;
    current_brush = gimp_brush_select_brush (brush,
                                            last_coords,
                                            current_coords);
    *last_coords = *current_coords;

    // brush_cache is managed by GimpBrush itself.
    dab_mask =
      gimp_brush_transform_mask (current_brush,
                                 scale, gimp_brush_aspect_ratio, -angle / 360,
                                 hardness);
    return true;
  }
  

  void 
  get_boundary(int&x1, int& y1, int& x2, int& y2) 
  {
    int radius_x  = (dab_mask->width + 1) / 2;
    int radius_y  = (dab_mask->height + 1) / 2;
    /* configure the pixel regions */
    x1  = floor(x - radius_x);
    y1  = floor(y - radius_y);
    x2  = floor(x1 + dab_mask->width - 1);
    y2  = floor(y1 + dab_mask->height - 1);
  };

  
  void 
  fill_brushmark_buffer (Pixel::real *dab_mask,
                         gint*, //offsets: unused
                         float ,//cx_in_tile: unused 
                         float ,//cy_in_tile: unused
                         PixelRegion* srcPR,
                         PixelRegion* channelPR,
                         PixelRegion* texturePR) 
  {
    Pixel::real*   mask_ptr;
    Pixel::data_t* data;

    mask_ptr = dab_mask;
    data = srcPR->data;
    for (int y = 0; y < srcPR->h; y ++) {
      for (int x = 0; x < srcPR->w; x ++) {
        *mask_ptr = eval(pix(data[x]));
        mask_ptr ++;
      }
      data += srcPR->rowstride;
    }

    if (channelPR) {
      mask_ptr = dab_mask;
      data = srcPR->data;
      Pixel::data_t* data2 = channelPR->data;
      for (int y = 0; y < srcPR->h; y ++) {
        for (int x = 0; x < srcPR->w; x ++) {
          *mask_ptr = eval(pix(*mask_ptr) * pix(data2[x]));
          mask_ptr ++;
        }
        data2 += channelPR->rowstride;
      }
    }

    if (texturePR) {
      mask_ptr = dab_mask;
      data = srcPR->data;
      Pixel::data_t* data2 = texturePR->data;
      for (int y = 0; y < srcPR->h; y ++) {
        for (int x = 0; x < srcPR->w; x ++) {
          *mask_ptr = eval(pix(*mask_ptr) * (pix(data2[x * texturePR->bytes]) + pix(texture_grain)) * pix(texture_contrast) );
          mask_ptr ++;
        }
        data2 += texturePR->rowstride;
      }
    }

  }


  void 
  draw_dab(PixelRegion* srcPR, 
           PixelRegion* destPR,
           PixelRegion* brushPR, 
           PixelRegion* maskPR,
           PixelRegion* texturePR)
  {
    Pixel::real dab_mask[(MAX(TILE_WIDTH, srcPR->w) + 2) * MAX(TILE_HEIGHT, srcPR->h)];

    Pixel::data_t* src_data  = srcPR->data;
    Pixel::data_t* dest_data = destPR->data; 
    
    fill_brushmark_buffer (dab_mask,
                           NULL,
                           x - srcPR->x,
                           y - srcPR->y,
                           brushPR,
                           maskPR, texturePR);

    iterator iter(dab_mask, fg_color, src_data, dest_data, 
                  srcPR->w, srcPR->h, brushPR->w, 
                  srcPR->rowstride, destPR->rowstride,
                  1, srcPR->bytes, destPR->bytes);

    Parent::draw_dab(iter);        
  }

  void
  copy_stroke(PixelRegion* src1PR, PixelRegion* destPR, 
              PixelRegion* brushPR, PixelRegion* maskPR,
              PixelRegion* texturePR) 
  {
    Parent::copy_stroke(src1PR, destPR, brushPR, maskPR, texturePR);
  }

  void
  get_color(PixelRegion* src1PR, 
            PixelRegion* brushPR, 
            PixelRegion* maskPR,
            PixelRegion* texturePR)
  {
    float sum_weight, sum_r, sum_g, sum_b, sum_a;
    sum_weight = sum_r = sum_g = sum_b = sum_a = 0.0;

    Pixel::real dab_mask[(MAX(TILE_WIDTH, src1PR->w) + 2) * MAX(TILE_HEIGHT, src1PR->h)];
    gint dab_offsets[(MAX(TILE_HEIGHT, src1PR->h) + 2)*2];

    Pixel::data_t*  src_data = src1PR->data;

    fill_brushmark_buffer (dab_mask, NULL,
                           x - src1PR->x, y - src1PR->y,
                           brushPR, maskPR, texturePR);

    iterator iter(dab_mask, fg_color,  src_data, src_data, 
                  src1PR->w, src1PR->h, brushPR->w, 
                  src1PR->rowstride, src1PR->rowstride,
                  1, src1PR->bytes, src1PR->bytes);

    get_color_pixels_accumulate (iter,
                                 &sum_weight, &sum_r, &sum_g, &sum_b, &sum_a);
    accumulator.accumulate(sum_weight, sum_r, sum_g, sum_b, sum_a);
  }
};

#endif
