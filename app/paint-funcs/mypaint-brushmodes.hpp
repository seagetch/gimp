/* This file is part of MyPaint.
 * Copyright (C) 2008-2011 by Martin Renold <martinxyz@gmx.ch>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define REAL_CALC
#include "base/pixel.hpp"
//using namespace Pixel::DataCalc;
// parameters to those methods:
//
// rgba: A pointer to 16bit rgba data with premultiplied alpha.
//       The range of each components is limited from 0 to 2^15.
//
// mask: Contains the dab shape, that is, the intensity of the dab at
//       each pixel. Usually rendering is done for one tile at a
//       time. The mask is LRE encoded to jump quickly over regions
//       that are not affected by the dab.
//
// opacity: overall strenght of the blending mode. Has the same
//          influence on the dab as the values inside the mask.


// We are manipulating pixels with premultiplied alpha directly.
// This is an "over" operation (opa = topAlpha).
// In the formula below, topColor is assumed to be premultiplied.
//
//               opa_a      <   opa_b      >
// resultAlpha = topAlpha + (1.0 - topAlpha) * bottomAlpha
// resultColor = topColor + (1.0 - topAlpha) * bottomColor
//
void draw_dab_pixels_BlendMode_Normal (Pixel::data_t * mask,
                                       gint          *offsets,
                                       Pixel::data_t *rgba,
                                       Pixel::data_t  color_r,
                                       Pixel::data_t  color_g,
                                       Pixel::data_t  color_b,
                                       Pixel::data_t  opacity,
                                       gint           bytes) {
  switch (bytes) {
  case 3:
    while (1) {
      for (; mask[0]; mask++, rgba+=3) {
        pixel_t opa_a = pix( eval( pix(mask[0]) * pix(opacity) ) ); // topAlpha
        pixel_t opa_b = pix( eval (f2p(1.0) - opa_a ) ); // bottomAlpha
        rgba[0] = eval( opa_a*pix(color_r) + opa_b*pix(rgba[0]) );
        rgba[1] = eval( opa_a*pix(color_g) + opa_b*pix(rgba[1]) );
        rgba[2] = eval( opa_a*pix(color_b) + opa_b*pix(rgba[2]) );
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 3;
      offsets ++;
      mask ++;
    }
    break;
  case 4:
    while (1) {
      for (; mask[0]; mask++, rgba+=4) {
        pixel_t brush_a = pix( eval( pix(mask[0]) * pix(opacity) ) );
        pixel_t base_a  = pix(rgba[3]);
        rgba[3] = eval( brush_a + (f2p(1.0) - brush_a)*base_a );
        if (rgba[3]) {
          pixel_t dest_a = pix(rgba[3]);
          rgba[0] = eval( (brush_a * pix(color_r) + (f2p(1.0) - brush_a) * base_a * pix(rgba[0])) / dest_a );
          rgba[1] = eval( (brush_a * pix(color_g) + (f2p(1.0) - brush_a) * base_a * pix(rgba[1])) / dest_a );
          rgba[2] = eval( (brush_a * pix(color_b) + (f2p(1.0) - brush_a) * base_a * pix(rgba[2])) / dest_a );
        } else {
          rgba[0] = color_r;
          rgba[1] = color_g;
          rgba[2] = color_b;
        }
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 4;
      offsets ++;
      mask ++;
    }
    break;
  default:
    g_print("Unsupported layer type.\n");
  }
};

// This blend mode is used for smudging and erasing.  Smudging
// allows to "drag" around transparency as if it was a color.  When
// smuding over a region that is 60% opaque the result will stay 60%
// opaque (color_a=0.6).  For normal erasing color_a is set to 0.0
// and color_r/g/b will be ignored. This function can also do normal
// blending (color_a=1.0).
//
void draw_dab_pixels_BlendMode_Normal_and_Eraser (Pixel::data_t * mask,
                                                  gint          *offsets,
                                                  Pixel::data_t * rgba,
                                                  Pixel::data_t color_r,
                                                  Pixel::data_t color_g,
                                                  Pixel::data_t color_b,
                                                  Pixel::data_t color_a,
                                                  Pixel::data_t opacity,
                                                  gint          bytes,
                                                  Pixel::data_t background_r = Pixel::from_f(1.0),
                                                  Pixel::data_t background_g = Pixel::from_f(1.0),
                                                  Pixel::data_t background_b = Pixel::from_f(1.0)) {
//  g_print("BlendMode_Normal_and_Eraser(color_a=%d)\n", color_a);

  switch (bytes) {
  case 3:
    while (1) {
      for (; mask[0]; mask++, rgba+=3) {
        pixel_t brush_a     = pix( eval( pix(mask[0]) * pix(opacity) ) ); // topAlpha
        pixel_t inv_brush_a = pix( eval (f2p(1.0) - brush_a ) ); // bottomAlpha
        rgba[0] = eval( brush_a*((pix(1.0f) - pix(color_a))*pix(background_r) + pix(color_a)*pix(color_r)) + inv_brush_a*pix(rgba[0]) );
        rgba[1] = eval( brush_a*((pix(1.0f) - pix(color_a))*pix(background_g) + pix(color_a)*pix(color_g)) + inv_brush_a*pix(rgba[1]) );
        rgba[2] = eval( brush_a*((pix(1.0f) - pix(color_a))*pix(background_b) + pix(color_a)*pix(color_b)) + inv_brush_a*pix(rgba[2]) );
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 3;
      offsets ++;
      mask ++;
    }
    break;
  case 4:
    while (1) {
      for (; mask[0]; mask++, rgba+=4) {
        pixel_t brush_a = pix( eval( pix(mask[0]) * pix(opacity) ) );
        pixel_t base_a  = pix(rgba[3]);
//        pixel_t orig_dest = pix(rgba[3]);
        rgba[3] = eval( brush_a * pix(color_a) + (pix(1.0f) - brush_a) * base_a );
        
        if (rgba[3]) {
          pixel_t inv_brush_a = pix( eval(pix(1.0f) - brush_a));
          pixel_t dest_a = pix(rgba[3]);
          rgba[0] = eval( (inv_brush_a*base_a*pix(rgba[0]) + brush_a*pix(color_a)*pix(color_r)) / dest_a );
          rgba[1] = eval( (inv_brush_a*base_a*pix(rgba[1]) + brush_a*pix(color_a)*pix(color_g)) / dest_a );
          rgba[2] = eval( (inv_brush_a*base_a*pix(rgba[2]) + brush_a*pix(color_a)*pix(color_b)) / dest_a );
        } else {
          rgba[0] = color_r;
          rgba[1] = color_g;
          rgba[2] = color_b;
        }
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 4;
      offsets ++;
      mask ++;
    }
    break;
  default:
    g_print("Unsupported layer type.\n");
  }
};

// This is BlendMode_Normal with locked alpha channel.
//
void draw_dab_pixels_BlendMode_LockAlpha (Pixel::data_t * mask,
                                          Pixel::data_t * rgba,
                                          Pixel::data_t color_r,
                                          Pixel::data_t color_g,
                                          Pixel::data_t color_b,
                                          Pixel::data_t opacity) {

  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      pixel_t opa_a = pix( eval( pix(mask[0]) * pix(opacity) ) ); // topAlpha
      pixel_t opa_b = pix( eval( f2p(1.0) - opa_a) ); // bottomAlpha
      pixel_t alpha = pix( rgba[3] );
          
      rgba[0] = eval(opa_a*alpha*pix(color_r) + opa_b*pix(rgba[0]) );
      rgba[1] = eval(opa_a*alpha*pix(color_g) + opa_b*pix(rgba[1]) );
      rgba[2] = eval(opa_a*alpha*pix(color_b) + opa_b*pix(rgba[2]) );
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};


// Sum up the color/alpha components inside the masked region.
// Called by get_color().
//
void get_color_pixels_accumulate (Pixel::data_t * mask,
                                  gint          *offsets,
                                  Pixel::data_t * rgba,
                                  float * sum_weight,
                                  float * sum_r,
                                  float * sum_g,
                                  float * sum_b,
                                  float * sum_a,
                                  gint bytes
                                  ) {


  // The sum of a 64x64 tile fits into a 32 bit integer, but the sum
  // of an arbitrary number of tiles may not fit. We assume that we
  // are processing a single tile at a time, so we can use integers.
  // But for the result we need floats.

  internal_t weight = 0;
  internal_t r = 0;
  internal_t g = 0;
  internal_t b = 0;
  internal_t a = 0;

  switch (bytes) {
  case 3:
    while (1) {
      for (; mask[0]; mask++, rgba+=3) {
        pixel_t opa = pix (mask[0]);
        weight += eval(opa);
        r      += eval (opa * pix(rgba[0]));
        g      += eval (opa * pix(rgba[1]));
        b      += eval (opa * pix(rgba[2]));
        a      += Pixel::from_f(1.0);
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 3;
      offsets ++;
      mask ++;
    }
    break;
  case 4:
    while (1) {
      for (; mask[0]; mask++, rgba+=4) {
        pixel_t opa = pix (mask[0]);
        weight += eval(opa);
        r      += eval (opa * pix(rgba[0]) * pix(rgba[3]));
        g      += eval (opa * pix(rgba[1]) * pix(rgba[3]));
        b      += eval (opa * pix(rgba[2]) * pix(rgba[3]));
        a      += eval (opa * pix(rgba[3]));
      }
      if (!offsets[0]) break;
      rgba += offsets[0] * 4;
      offsets ++;
      mask ++;
    }
    break;
  default:
    g_print("Unsupported layer type.\n");
  }

  // convert integer to float outside the performance critical loop
  *sum_weight += weight;
  *sum_r += r;
  *sum_g += g;
  *sum_b += b;
  *sum_a += a;
};

#if 0
// Colorize: apply the source hue and saturation, retaining the target
// brightness. Same thing as in the PDF spec addendum but with different Luma
// coefficients. Colorize should be used at either 1.0 or 0.0, values in
// between probably aren't very useful. This blend mode retains the target
// alpha, and any pure whites and blacks in the target layer.

#define MAX3(a, b, c) ((a)>(b)?MAX((a),(c)):MAX((b),(c)))
#define MIN3(a, b, c) ((a)<(b)?MIN((a),(c)):MIN((b),(c)))

/*
// From ITU Rec. BT.601 (SDTV)
static const float LUMA_RED_COEFF   = 0.3;
static const float LUMA_GREEN_COEFF = 0.59;
static const float LUMA_BLUE_COEFF  = 0.11;
*/

// From ITU Rec. BT.709 (HDTV and sRGB)
static const uint16_t LUMA_RED_COEFF = 0.2126 * (1<<15);
static const uint16_t LUMA_GREEN_COEFF = 0.7152 * (1<<15);
static const uint16_t LUMA_BLUE_COEFF = 0.0722 * (1<<15);

// See also http://en.wikipedia.org/wiki/YCbCr


/* Returns the sRGB luminance of an RGB triple, expressed as scaled ints. */

#define LUMA(r,g,b) \
   ((r)*LUMA_RED_COEFF + (g)*LUMA_GREEN_COEFF + (b)*LUMA_BLUE_COEFF)


/*
 * Sets the output RGB triple's luminance to that of the input, retaining its
 * colour. Inputs and outputs are scaled ints having factor 2**-15, and must
 * not store premultiplied alpha.
 */

inline static void
set_rgb16_lum_from_rgb16(const uint16_t topr,
                         const uint16_t topg,
                         const uint16_t topb,
                         uint16_t *botr,
                         uint16_t *botg,
                         uint16_t *botb)
{
    // Spec: SetLum()
    // Colours potentially can go out of band to both sides, hence the
    // temporary representation inflation.
    const uint16_t botlum = LUMA(*botr, *botg, *botb) / (1<<15);
    const uint16_t toplum = LUMA(topr, topg, topb) / (1<<15);
    const int16_t diff = botlum - toplum;
    int32_t r = topr + diff;
    int32_t g = topg + diff;
    int32_t b = topb + diff;

    // Spec: ClipColor()
    // Clip out of band values
    int32_t lum = LUMA(r, g, b) / (1<<15);
    int32_t cmin = MIN3(r, g, b);
    int32_t cmax = MAX3(r, g, b);
    if (cmin < 0) {
        r = lum + (((r - lum) * lum) / (lum - cmin));
        g = lum + (((g - lum) * lum) / (lum - cmin));
        b = lum + (((b - lum) * lum) / (lum - cmin));
    }
    if (cmax > (1<<15)) {
        r = lum + (((r - lum) * ((1<<15)-lum)) / (cmax - lum));
        g = lum + (((g - lum) * ((1<<15)-lum)) / (cmax - lum));
        b = lum + (((b - lum) * ((1<<15)-lum)) / (cmax - lum));
    }
#ifdef HEAVY_DEBUG
    assert((0 <= r) && (r <= (1<<15)));
    assert((0 <= g) && (g <= (1<<15)));
    assert((0 <= b) && (b <= (1<<15)));
#endif

    *botr = r;
    *botg = g;
    *botb = b;
}


// The method is an implementation of that described in the official Adobe "PDF
// Blend Modes: Addendum" document, dated January 23, 2006; specifically it's
// the "Color" nonseparable blend mode. We do however use different
// coefficients for the Luma value.

void
draw_dab_pixels_BlendMode_Color (uint16_t *mask,
                                 uint16_t *rgba, // b=bottom, premult
                                 uint16_t color_r,  // }
                                 uint16_t color_g,  // }-- a=top, !premult
                                 uint16_t color_b,  // }
                                 uint16_t opacity)
{
  while (1) {
    for (; mask[0]; mask++, rgba+=4) {
      // De-premult
      uint16_t r, g, b;
      const uint16_t a = rgba[3];
      r = g = b = 0;
      if (rgba[3] != 0) {
        r = ((1<<15)*((uint32_t)rgba[0])) / a;
        g = ((1<<15)*((uint32_t)rgba[1])) / a;
        b = ((1<<15)*((uint32_t)rgba[2])) / a;
      }

      // Apply luminance
      set_rgb16_lum_from_rgb16(color_r, color_g, color_b, &r, &g, &b);

      // Re-premult
      r = ((uint32_t) r) * a / (1<<15);
      g = ((uint32_t) g) * a / (1<<15);
      b = ((uint32_t) b) * a / (1<<15);

      // And combine as normal.
      uint32_t opa_a = mask[0] * opacity / (1<<15); // topAlpha
      uint32_t opa_b = (1<<15) - opa_a; // bottomAlpha
      rgba[0] = (opa_a*r + opa_b*rgba[0])/(1<<15);
      rgba[1] = (opa_a*g + opa_b*rgba[1])/(1<<15);
      rgba[2] = (opa_a*b + opa_b*rgba[2])/(1<<15);
    }
    if (!mask[1]) break;
    rgba += mask[1];
    mask += 2;
  }
};
#endif

