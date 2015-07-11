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

const float ALPHA_THRESHOLD = (float)((1 << 16) - 1);

////////////////////////////////////////////////////////////////////////////////
template<typename PixelIter>
class GimpMypaintSurfaceForGeneralBrush {
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
public:
  typedef PixelIter iterator;
  bool 
  prepare_brush(float x, 
                float y, 
                float radius, 
                float hardness, 
                float aspect_ratio, 
                float angle, 
                float normal, 
                float opaque, 
                float lock_alpha, 
                Pixel::real* fg_color, 
                float color_a, 
                Pixel::real* bg_color, 
                float stroke_opacity,
                float texture_grain,
                float texture_contrast,
                void* /*unused*/)
  {
    this->x            = x;
    this->y            = y;
    this->normal       = normal;
    this->opaque       = opaque;
    this->lock_alpha   = lock_alpha;
    this->color_a      = color_a;
    this->fg_color[0]  = fg_color[0];
    this->fg_color[1]  = fg_color[1];
    this->fg_color[2]  = fg_color[2];
    this->fg_color[3]  = fg_color[3];
    this->bg_color[0]  = bg_color[0];
    this->bg_color[1]  = bg_color[1];
    this->bg_color[2]  = bg_color[2];
    this->stroke_opacity = stroke_opacity;
    this->texture_grain = texture_grain;
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
  copy_stroke(PixelRegion* src1PR, 
              PixelRegion* destPR, 
              PixelRegion* brushPR, 
              PixelRegion* maskPR,
              PixelRegion* texturePR) 
  {
    BrushPixelIteratorForPlainData<PixmapBrushmarkIterator, Pixel::data_t, Pixel::data_t>
       iter(brushPR->data, NULL, 
            src1PR->data, destPR->data, 
            src1PR->w, src1PR->h, 
            brushPR->rowstride, 
            src1PR->rowstride,
            destPR->rowstride,
            brushPR->bytes,
            src1PR->bytes,
            destPR->bytes);
    // normal case for brushes that use smudging (eg. watercolor)
    draw_dab_pixels_BlendMode_Normal_and_Eraser(iter,
                                                color_a,
                                                stroke_opacity,
                                                bg_color[0], bg_color[1], bg_color[2]);
//        draw_dab_pixels_BlendMode_Normal(iter, 0.4);
  }

};


////////////////////////////////////////////////////////////////////////////////
class GimpMypaintSurfaceForMypaintBrush : 
  public GimpMypaintSurfaceForGeneralBrush<BrushPixelIteratorForRunLength>
{
  float radius;
  float hardness, aspect_ratio, angle;

public:
  typedef BrushPixelIteratorForRunLength iterator;
  typedef GimpMypaintSurfaceForGeneralBrush<iterator> Parent;

  bool 
  prepare_brush(float x, 
                float y, 
                float radius, 
                float hardness, 
                float aspect_ratio, 
                float angle, 
                float normal, 
                float opaque, 
                float lock_alpha, 
                Pixel::real* fg_color, 
                float color_a, 
                Pixel::real* bg_color, 
                float stroke_opacity,
                float texture_grain,
                float texture_contrast,
                void* brush)
  {
    Parent::prepare_brush(x, y, 
                          radius, 
                          hardness, 
                          aspect_ratio, 
                          angle, 
                          normal, 
                          opaque, 
                          lock_alpha, 
                          fg_color, 
                          color_a, 
                          bg_color, stroke_opacity,
                          texture_grain, texture_contrast,
                          brush);
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
    Pixel::real dab_mask[TILE_WIDTH*TILE_HEIGHT+2];
    gint dab_offsets[TILE_WIDTH * TILE_HEIGHT];

    Pixel::data_t* src_data  = src1PR->data;
    Pixel::data_t* dest_data = destPR->data;

    fill_brushmark_buffer (dab_mask,
                           dab_offsets,
                           x - src1PR->x,
                           y - src1PR->y,
                           src1PR,
                           maskPR,
                           texturePR);

    BrushPixelIteratorForRunLength iter(dab_mask, dab_offsets, fg_color, src_data, dest_data, src1PR->bytes, destPR->bytes);
    
    Parent::draw_dab(iter);
  }
};

////////////////////////////////////////////////////////////////////////////////
class GimpMypaintSurfaceForGimpBrush : 
  public GimpMypaintSurfaceForGeneralBrush<BrushPixelIteratorForPlainData<ColoredBrushmarkIterator, Pixel::real, Pixel::real> >
{
  GimpCoords* last_coords;
  GimpCoords* current_coords;
  const TempBuf* dab_mask;
  float radius;
  
public:
  typedef BrushPixelIteratorForPlainData<ColoredBrushmarkIterator, Pixel::real, Pixel::real> iterator;
  typedef GimpMypaintSurfaceForGeneralBrush<iterator> Parent;
  GimpMypaintSurfaceForGimpBrush(GimpCoords* current_coords,
                                 GimpCoords* last_coords)
    : dab_mask(NULL)
  {
    this->current_coords = current_coords;
    this->last_coords    = last_coords;
  };

  const TempBuf* get_brush_data()
  { 
    return dab_mask;
  }


  bool 
  prepare_brush(float x, 
                float y, 
                float radius, 
                float hardness, 
                float aspect_ratio, 
                float angle, 
                float normal, 
                float opaque, 
                float lock_alpha, 
                Pixel::real* fg_color, 
                float color_a, 
                Pixel::real* bg_color, 
                float stroke_opacity,
                float texture_grain, float texture_contrast,
                void* data)
  {
    Parent::prepare_brush(x, y, 
                          radius, 
                          hardness, 
                          aspect_ratio, 
                          angle, 
                          normal, 
                          opaque, 
                          lock_alpha, 
                          fg_color, 
                          color_a, 
                          bg_color,
                          stroke_opacity,
                          texture_grain, texture_contrast,
                          data);
    dab_mask         = NULL;
    GimpBrush* brush = GIMP_BRUSH(data);
    this->radius     = radius;
    int brush_radius = MAX(brush->mask->width, brush->mask->height);

    if (brush_radius < 1) 
      return false;

    float scale                   = radius * 2 / brush_radius;
    float gimp_brush_aspect_ratio = 20 * (1.0 - ( 1.0 / aspect_ratio));

    GimpBrush* current_brush;
    current_brush = gimp_brush_select_brush (brush,
                                            last_coords,
                                            current_coords);

    *last_coords = *current_coords;

    dab_mask=
      gimp_brush_transform_mask (current_brush,
                                 scale,
                                 gimp_brush_aspect_ratio,
                                 -angle / 360,
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
    Pixel::real dab_mask[TILE_WIDTH*TILE_HEIGHT+2*TILE_HEIGHT];

    Pixel::data_t* src_data  = srcPR->data;
    Pixel::data_t* dest_data = destPR->data; 
    
    fill_brushmark_buffer (dab_mask,
                           NULL,
                           x - srcPR->x,
                           y - srcPR->y,
                           brushPR,
                           maskPR, texturePR);

    iterator iter(dab_mask, fg_color, 
                  src_data, dest_data, 
                  srcPR->w, srcPR->h, 
                  brushPR->w, 
                  srcPR->rowstride,
                  destPR->rowstride,
                      1,
                  srcPR->bytes,
                  destPR->bytes);

    Parent::draw_dab(iter);        
  }

};

////////////////////////////////////////////////////////////////////////////////
#define DefineRegionProcessor(CLASS, FUNC) \
template<typename MypaintSurfaceImpl> \
class CLASS { \
public: \
  static void process_src_dest(MypaintSurfaceImpl* impl, \
                               PixelRegion* srcPR, \
                               PixelRegion* destPR) \
  { \
    impl->FUNC(srcPR, destPR, NULL, NULL, NULL); \
  } \
 \
  static void process_src_dest_mask(MypaintSurfaceImpl* impl,  \
                                    PixelRegion* srcPR,  \
                                    PixelRegion* destPR,  \
                                    PixelRegion* maskPR) \
  { \
    impl->FUNC(srcPR, destPR, NULL, maskPR, NULL); \
  } \
 \
  static void process_src_dest_brush(MypaintSurfaceImpl* impl, \
                                     PixelRegion* srcPR,  \
                                     PixelRegion* destPR, \
                                     PixelRegion* brushPR) \
  { \
    impl->FUNC(srcPR, destPR, brushPR, NULL, NULL); \
  } \
 \
  static void process_full(MypaintSurfaceImpl* impl, \
                           PixelRegion* srcPR, \
                           PixelRegion* destPR, \
                           PixelRegion* brushPR, \
                           PixelRegion* maskPR, \
                           PixelRegion* texturePR) \
  { \
    impl->FUNC(srcPR, destPR, brushPR, maskPR, texturePR); \
  } \
 \
  void \
  process(MypaintSurfaceImpl* impl, \
          PixelRegion* srcPR, \
          PixelRegion* destPR, \
          PixelRegion* brushPR, \
          PixelRegion* maskPR, \
          PixelRegion* texturePR) \
  { \
    pixel_regions_process_parallel((PixelProcessorFunc)CLASS::process_full, \
                                   impl, \
                                   5, \
                                   srcPR, destPR, brushPR, maskPR, texturePR); \
  }; \
      \
};

DefineRegionProcessor(DrawDabProcessor, draw_dab);
DefineRegionProcessor(CopyStrokeProcessor, copy_stroke);

////////////////////////////////////////////////////////////////////////////////
class GimpMypaintSurfaceImpl : public GimpMypaintSurface
{
private:
  virtual GimpUndo* push_undo (GimpImage *imag, const gchar* undo_desc);
  GimpDrawable* drawable;
  GimpRGB       bg_color;
  GimpBrush*    brushmark;
  GimpPattern*  texture;
  GimpCoords    last_coords;
  GimpCoords    current_coords;
  bool          floating_stroke;
  float         stroke_opacity;
  
  TileManager* undo_tiles;       /*  tiles which have been modified      */
  TileManager* floating_stroke_tiles;

  gint         x1, y1;           /*  undo extents in image coords        */
  gint         x2, y2;           /*  undo extents in image coords        */
  gint         session;          /*  reference counter of atomic scope   */

  void      validate_undo_tiles       (gint              x,
                                       gint              y,
                                       gint              w,
                                       gint              h);

  void      validate_floating_stroke_tiles (gint              x,
                                            gint              y,
                                            gint              w,
                                            gint              h);

  void start_undo_group();
  void stop_updo_group();
  
  void start_floating_stroke();
  void stop_floating_stroke();
public:
  GimpMypaintSurfaceImpl(GimpDrawable* d) 
    : undo_tiles(NULL), floating_stroke_tiles(NULL), 
      session(0), drawable(d), brushmark(NULL), 
      floating_stroke(false), stroke_opacity(1.0), texture(NULL)
  {
    g_object_add_weak_pointer(G_OBJECT(d), (gpointer*)&drawable);
  }

  virtual ~GimpMypaintSurfaceImpl()
  {
    if (drawable)
      g_object_remove_weak_pointer(G_OBJECT(drawable), (gpointer*)&drawable);
    if (undo_tiles) {
      tile_manager_unref (undo_tiles);
      undo_tiles = NULL;
    }
    if (floating_stroke_tiles) {
      tile_manager_unref (floating_stroke_tiles);
      floating_stroke_tiles = NULL;
    }
    if (brushmark)
      g_object_unref(G_OBJECT(brushmark));

    if (texture)
      g_object_unref(G_OBJECT(texture));
  }


  virtual bool draw_dab (float x, float y, 
                         float radius, 
                         float color_r, float color_g, float color_b,
                         float opaque, float hardness = 0.5,
                         float alpha_eraser = 1.0,
                         float aspect_ratio = 1.0, float angle = 0.0,
                         float lock_alpha = 0.0, float colorize = 0.0,
                         float texture_grain = 0.0, float texture_contrast = 1.0
                         );

  template<class BrushImpl>
  bool draw_dab_impl (BrushImpl& brush_impl,
                      float x, float y, 
                      float radius, 
                      float color_r, float color_g, float color_b,
                      float opaque, float hardness,
                      float color_a,
                      float aspect_ratio, float angle,
                      float lock_alpha,float colorize,
                      float texture_grain = 0.0, float texture_contrast = 1.0)
  {
    GimpItem        *item  = GIMP_ITEM (drawable);
    GimpImage       *image = gimp_item_get_image (item);
    GimpChannel     *mask  = gimp_image_get_mask (image);
    gint            offset_x, offset_y;
    PixelRegion     src1PR, destPR;
    PixelRegion     brushPR;
    PixelRegion     maskPR;
    PixelRegion     texturePR;
    
    opaque     = CLAMP(opaque, 0.0, 1.0);
    hardness   = CLAMP(hardness, 0.0, 1.0);
    lock_alpha = CLAMP(lock_alpha, 0.0, 1.0);
    colorize   = CLAMP(colorize, 0.0, 1.0);

    if (radius < 0.1) return false; // don't bother with dabs smaller than 0.1 pixel
    if (hardness == 0.0) return false; // infintly small center point, fully transparent outside
    if (opaque == 0.0) return false;

    if (aspect_ratio<1.0) aspect_ratio=1.0;

    /*  get the layer offsets  */
    gimp_item_get_offset (item, &offset_x, &offset_y);

    // blending mode preparation
    float normal = 1.0;

    normal *= 1.0-lock_alpha;
    normal *= 1.0-colorize;

    Pixel::real fg_color[4];
    fg_color[0] = color_r;
    fg_color[1] = color_g;
    fg_color[2] = color_b;
    fg_color[3] = color_a;
    Pixel::real bg_color[3];
    bg_color[0] = this->bg_color.r;
    bg_color[1] = this->bg_color.g;
    bg_color[2] = this->bg_color.b;

    if (!brush_impl.prepare_brush(x, y, radius, 
                                  hardness, aspect_ratio, angle, 
                                  normal, opaque, lock_alpha,
                                  fg_color, color_a, bg_color, stroke_opacity,
                                  texture_grain, texture_contrast,
                                  (void*)brushmark))
      return false;

    int x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    brush_impl.get_boundary(x1, y1, x2, y2);

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

    TempBuf* dab_mask = (TempBuf*)brush_impl.get_brush_data();

    if (dab_mask) {
      if (dab_mask->width < rx2 - rx1 + 1)
        rx2 = rx1 + dab_mask->width - 1;
      if (dab_mask->height < ry2 - ry1 + 1)
        ry2 = ry1 + dab_mask->height - 1;
    }

    int width    = (rx2 - rx1 + 1);
    int height   = (ry2 - ry1 + 1);

    if (rx1 > rx2 || ry1 > ry2)
      return false;

    if (dab_mask &&
        (dab_mask->width < rx1 - x1 || dab_mask->height < ry1 - y1))
      return false;

    /*  set undo blocks  */
    start_undo_group();
    validate_undo_tiles (rx1, ry1, width, height);

    if (floating_stroke) {
      validate_floating_stroke_tiles(rx1, ry1, width, height);

      pixel_region_init (&src1PR, floating_stroke_tiles,
                         rx1, ry1, width, height,
                         FALSE);

      pixel_region_init (&destPR, floating_stroke_tiles,
                         rx1, ry1, width, height,
                         FALSE);
    } else {
      pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                         rx1, ry1, width, height,
                         TRUE);

      pixel_region_init (&destPR, gimp_drawable_get_tiles (drawable),
                         rx1, ry1, width, height,
                         TRUE);
     }

    TempBuf* pattern = NULL;
    if (texture) {
      pattern = gimp_pattern_get_mask (texture);
    }

    if (dab_mask)
      pixel_region_init_temp_buf(&brushPR, dab_mask, 
                                 MAX(rx1 - x1, 0), 
                                 MAX(ry1 - y1, 0), 
                                 width, height);

    if (mask_item) {
      pixel_region_init (&maskPR,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                         rx1 + offset_x,
                         ry1 + offset_y,
                         width, height,
                         TRUE);
     }

    if (pattern) {
      pixel_region_init_temp_buf(&texturePR, pattern,
                                 rx1 % pattern->width,
                                 ry1 % pattern->height,
                                 pattern->width, pattern->height);
      pixel_region_set_closed_loop(&texturePR, TRUE);
    }
    
    DrawDabProcessor<BrushImpl> processor;
    processor.process(&brush_impl,
                     &src1PR, 
                     &destPR, 
                     (dab_mask)? &brushPR: NULL, 
                     (mask_item)? &maskPR: NULL,
                     (pattern)? &texturePR: NULL);

    if (floating_stroke) {
      /* Copy floating stroke buffer into drawable buffer */
      pixel_region_init (&src1PR, undo_tiles,
                         rx1, ry1, width, height,
                         TRUE);

      pixel_region_init (&destPR, gimp_drawable_get_tiles(drawable),
                         rx1, ry1, width, height,
                         TRUE);

      pixel_region_init (&brushPR, floating_stroke_tiles,
                         rx1, ry1, width, height,
                         FALSE);

      if (mask_item) {
        pixel_region_init (&maskPR,
                           gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                           rx1 + offset_x,
                           ry1 + offset_y,
                           rx2 - rx1, ry2 - ry1,
                           TRUE);
        }
      
      CopyStrokeProcessor<BrushImpl> stroke_processor;
      stroke_processor.process(&brush_impl,
                               &src1PR,
                               &destPR,
                               &brushPR,
                               NULL, NULL);
    }

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


  virtual void get_color (float x, float y, 
                          float radius, 
                          float * color_r, float * color_g, float * color_b, float * color_a,
                          float texture_grain, float texture_contrast
                          );

  virtual void begin_session();
  virtual void end_session();
  
  bool is_surface_for (GimpDrawable* drawable) { return drawable == this->drawable; }
  void set_bg_color (GimpRGB* src) 
  { 
    if (src)
      bg_color = *src; 
  }

  void get_bg_color (GimpRGB* dest) 
  {
    if (dest)
      *dest = bg_color; 
  }

  void set_brushmark(GimpBrush* brush_)
  {
    if (brushmark) {
      gimp_brush_end_use(brushmark);
      g_object_unref(G_OBJECT(brushmark));
      brushmark = NULL;
    }

    if (brush_) {
      brushmark = brush_;
      gimp_brush_begin_use(brushmark);
      g_object_ref(G_OBJECT(brushmark));
    }
  }

  GimpBrush* get_brushmark()
  {
    return brushmark;
  }

  void set_texture(GimpPattern* texture_)
  {
    if (texture) {
      g_object_unref(G_OBJECT(texture));
      texture = NULL;
    }

    if (texture_) {
      texture = texture_;
      g_object_ref(G_OBJECT(texture));
    }
  }

  GimpPattern* get_texture()
  {
    return texture;
  }

  void set_floating_stroke(bool value) {
    floating_stroke = value;
  }

  bool get_floating_stroke () {
    return floating_stroke;
  }

  void set_stroke_opacity(double value) {
    stroke_opacity = (float)CLAMP(value, 0.0, 1.0);
  }

  virtual void set_coords(const GimpCoords* coords) { current_coords = *coords; }
};



bool
GimpMypaintSurfaceImpl::draw_dab (float x, float y, 
                                  float radius, 
                                  float color_r, float color_g, float color_b,
                                  float opaque, float hardness,
                                  float color_a,
                                  float aspect_ratio, float angle,
                                  float lock_alpha,
                                  float colorize,
                                  float texture_grain,
                                  float texture_contrast)
{
  if (brushmark) {
//	 g_print("GimpBrush::draw_dab_impl@%4f,%4f\n", x, y);
    GimpMypaintSurfaceForGimpBrush brush_impl(&current_coords, &last_coords);
    return draw_dab_impl(brush_impl,
                          x, y, radius, color_r, color_g, color_b, opaque,
                          hardness, color_a, aspect_ratio, angle, lock_alpha,
                          colorize, texture_grain, texture_contrast);
  } else {
//	 g_print("MypaintBrush::draw_dab_impl@%4f,%4f\n",x,y);
    GimpMypaintSurfaceForMypaintBrush brush_impl;
    return draw_dab_impl(brush_impl,
                          x, y, radius, color_r, color_g, color_b, opaque,
                          hardness, color_a, aspect_ratio, angle, lock_alpha,
                          colorize, texture_grain, texture_contrast);
  }
}

void 
GimpMypaintSurfaceImpl::get_color (float x, float y, 
                                   float radius, 
                                   float * color_r, 
                                   float * color_g, 
                                   float * color_b, 
                                   float * color_a,
                                   float texture_grain,
                                   float texture_contrast)
{
  GimpMypaintSurfaceForMypaintBrush brush_impl;

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
  Pixel::real fg_color[] = {0.0, 0.0, 0.0, 1.0};
  Pixel::real bg_color[] = {1.0, 1.0, 1.0};
  if (!brush_impl.prepare_brush(x, y, radius, 
                                hardness, aspect_ratio, angle, 
                                1.0, 1.0, 0.0,
                                fg_color, 1.0, bg_color, stroke_opacity,
                                texture_grain, texture_contrast,
                                brushmark))
      return;

  int x1, y1, x2, y2;
  brush_impl.get_boundary(x1, y1, x2, y2);

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
  if (floating_stroke) {
  
    pixel_region_init (&src1PR, floating_stroke_tiles,
                       rx1, ry1, width, height,
                       FALSE);

  } else {
    pixel_region_init (&src1PR, gimp_drawable_get_tiles (drawable),
                       rx1, ry1, width, height,
                       FALSE);
  }

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
  Pixel::real dab_mask[TILE_WIDTH*TILE_HEIGHT+2*TILE_HEIGHT];
  gint dab_offsets[(TILE_HEIGHT + 2)*2];
  for (;
       pr != NULL;
       pr = pixel_regions_process ((PixelRegionIterator*)pr)) {
    Pixel::data_t*  src_data = src1PR.data;
    

    brush_impl.fill_brushmark_buffer(dab_mask,
                                     dab_offsets,
                                     x - src1PR.x,
                                     y - src1PR.y,
                                     &src1PR, (mask_item)? &maskPR : NULL, NULL
                                     );

    BrushPixelIteratorForRunLength iter(dab_mask, dab_offsets, NULL, src_data, src_data,  src1PR.bytes, src1PR.bytes);

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
GimpMypaintSurfaceImpl::begin_session()
{
  session = 0;
  if (floating_stroke)
    start_floating_stroke();
}

void 
GimpMypaintSurfaceImpl::end_session()
{
  if (session <= 0)
    return;
    
  stop_updo_group();
  if (floating_stroke)
    stop_floating_stroke();
  session = 0;
}

void 
GimpMypaintSurfaceImpl::start_undo_group()
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
GimpMypaintSurfaceImpl::stop_updo_group()
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
GimpMypaintSurfaceImpl::push_undo (GimpImage     *image,
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

void 
GimpMypaintSurfaceImpl::start_floating_stroke()
{
  g_return_if_fail(drawable);
  GimpItem* item = GIMP_ITEM (drawable);

  if (floating_stroke_tiles) {
    tile_manager_unref(floating_stroke_tiles);
    floating_stroke_tiles = NULL;
  }

  gint bytes = gimp_drawable_bytes_with_alpha (drawable);
  floating_stroke_tiles = tile_manager_new(gimp_item_get_width(item),
                                           gimp_item_get_height(item),
                                           bytes);

}

void 
GimpMypaintSurfaceImpl::stop_floating_stroke()
{
  if (floating_stroke_tiles) {
    tile_manager_unref(floating_stroke_tiles);
    floating_stroke_tiles = NULL;
  }
}

void
GimpMypaintSurfaceImpl::validate_floating_stroke_tiles(gint x, 
                                                   gint y,
                                                   gint w,
                                                   gint h)
{
  gint i, j;

  g_return_if_fail (floating_stroke_tiles != NULL);

  for (i = y; i < (y + h); i += (TILE_HEIGHT - (i % TILE_HEIGHT))) {
    for (j = x; j < (x + w); j += (TILE_WIDTH - (j % TILE_WIDTH))) {
      Tile *tile = tile_manager_get_tile (floating_stroke_tiles, j, i,
                                          FALSE, FALSE);

      if (! tile_is_valid (tile)) {
        tile = tile_manager_get_tile (floating_stroke_tiles, j, i,
                                      TRUE, TRUE);
        memset (tile_data_pointer (tile, 0, 0), 0, tile_size (tile));
        tile_release (tile, TRUE);
      }
      
    }
  }

}

void
GimpMypaintSurfaceImpl::validate_undo_tiles (
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

      }
    }
  }
  
}

GimpMypaintSurface* GimpMypaintSurface_new(GimpDrawable* drawable)
{
  return new GimpMypaintSurfaceImpl(drawable);
}
