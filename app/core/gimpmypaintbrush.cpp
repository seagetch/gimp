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

#include <glib-object.h>
#include <cairo.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"

#include "base/temp-buf.h"

#include "gimpbezierdesc.h"
#include "gimpmypaintbrush.h"
#include "gimpmypaintbrush-private.hpp"
#include "gimpmypaintbrush-load.h"
#include "gimpmarshal.h"
#include "gimptagged.h"

#include "gimp-intl.h"


enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
};


static void          gimp_mypaint_brush_tagged_iface_init     (GimpTaggedInterface  *iface);

static void          gimp_mypaint_brush_finalize              (GObject              *object);
static void          gimp_mypaint_brush_set_property          (GObject              *object,
                                                       guint                 property_id,
                                                       const GValue         *value,
                                                       GParamSpec           *pspec);
static void          gimp_mypaint_brush_get_property          (GObject              *object,
                                                       guint                 property_id,
                                                       GValue               *value,
                                                       GParamSpec           *pspec);

static gint64        gimp_mypaint_brush_get_memsize           (GimpObject           *object,
                                                       gint64               *gui_size);

static gboolean      gimp_mypaint_brush_get_size              (GimpViewable         *viewable,
                                                       gint                 *width,
                                                       gint                 *height);
static TempBuf     * gimp_mypaint_brush_get_new_preview       (GimpViewable         *viewable,
                                                       GimpContext          *context,
                                                       gint                  width,
                                                       gint                  height);
static gchar       * gimp_mypaint_brush_get_description       (GimpViewable         *viewable,
                                                       gchar               **tooltip);

static void          gimp_mypaint_brush_dirty                 (GimpData             *data);
static const gchar * gimp_mypaint_brush_get_extension         (GimpData             *data);

static void          gimp_mypaint_brush_real_begin_use        (GimpMypaintBrush            *mypaint_brush);
static void          gimp_mypaint_brush_real_end_use          (GimpMypaintBrush            *mypaint_brush);
static GimpMypaintBrush   * gimp_mypaint_brush_real_select_mypaint_brush     (GimpMypaintBrush            *mypaint_brush,
                                                       const GimpCoords     *last_coords,
                                                       const GimpCoords     *current_coords);
static gboolean      gimp_mypaint_brush_real_want_null_motion (GimpMypaintBrush            *mypaint_brush,
                                                       const GimpCoords     *last_coords,
                                                       const GimpCoords     *current_coords);

static gchar       * gimp_mypaint_brush_get_checksum          (GimpTagged           *tagged);


G_DEFINE_TYPE_WITH_CODE (GimpMypaintBrush, gimp_mypaint_brush, GIMP_TYPE_DATA,
                         G_IMPLEMENT_INTERFACE (GIMP_TYPE_TAGGED,
                                                gimp_mypaint_brush_tagged_iface_init))

#define parent_class gimp_mypaint_brush_parent_class

//static guint mypaint_brush_signals[LAST_SIGNAL] = { 0 };


static void
gimp_mypaint_brush_class_init (GimpMypaintBrushClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpDataClass     *data_class        = GIMP_DATA_CLASS (klass);

#if 0
  mypaint_brush_signals[SPACING_CHANGED] =
    g_signal_new ("spacing-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (GimpMypaintBrushClass, spacing_changed),
                  NULL, NULL,
                  gimp_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
#endif
  object_class->finalize           = gimp_mypaint_brush_finalize;
  object_class->get_property       = gimp_mypaint_brush_get_property;
  object_class->set_property       = gimp_mypaint_brush_set_property;

  gimp_object_class->get_memsize   = gimp_mypaint_brush_get_memsize;

  viewable_class->default_stock_id = "gimp-tool-mypaint-brush";
  viewable_class->get_size         = gimp_mypaint_brush_get_size;
  viewable_class->get_new_preview  = gimp_mypaint_brush_get_new_preview;
  viewable_class->get_description  = gimp_mypaint_brush_get_description;

  data_class->dirty                = gimp_mypaint_brush_dirty;
  data_class->get_extension        = gimp_mypaint_brush_get_extension;

  klass->begin_use                 = gimp_mypaint_brush_real_begin_use;
  klass->end_use                   = gimp_mypaint_brush_real_end_use;
  klass->select_mypaint_brush              = gimp_mypaint_brush_real_select_mypaint_brush;
  klass->want_null_motion          = gimp_mypaint_brush_real_want_null_motion;
#if 0
  klass->spacing_changed           = NULL;
  g_object_class_install_property (object_class, PROP_SPACING,
                                   g_param_spec_double ("spacing", NULL,
                                                        _("MypaintBrush Spacing"),
                                                        1.0, 5000.0, 20.0,
                                                        (GParamFlags)(GIMP_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT)));
#endif
}

static void
gimp_mypaint_brush_tagged_iface_init (GimpTaggedInterface *iface)
{
  iface->get_checksum = gimp_mypaint_brush_get_checksum;
}

static void
gimp_mypaint_brush_init (GimpMypaintBrush *mypaint_brush)
{
//  mypaint_brush->pixmap   = NULL;
  mypaint_brush->icon     = NULL;

  mypaint_brush->p = (gpointer)(new GimpMypaintBrushPrivate());
}

static void
gimp_mypaint_brush_finalize (GObject *object)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (object);

  if (mypaint_brush->p) {
    GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(mypaint_brush->p);
    delete priv;
    mypaint_brush->p = NULL;
  }

/*
  if (mypaint_brush->pixmap) {
    temp_buf_free (mypaint_brush->pixmap);
    mypaint_brush = NULL;
  }
*/
  if (mypaint_brush->icon) {
    cairo_surface_destroy (mypaint_brush->icon);
  }
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gimp_mypaint_brush_set_property (GObject      *object,
                         guint         property_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
gimp_mypaint_brush_get_property (GObject    *object,
                         guint       property_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (object);

  switch (property_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
gimp_mypaint_brush_get_memsize (GimpObject *object,
                        gint64     *gui_size)
{
  GimpMypaintBrush *mypaint_brush   = GIMP_MYPAINT_BRUSH (object);
  gint64     memsize = 0;

//  memsize += temp_buf_get_memsize (mypaint_brush->mask);
//  memsize += temp_buf_get_memsize (mypaint_brush->pixmap);

  return memsize + GIMP_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static gboolean
gimp_mypaint_brush_get_size (GimpViewable *viewable,
                     gint         *width,
                     gint         *height)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (viewable);

//  *width  = mypaint_brush->mask->width;
//  *height = mypaint_brush->mask->height;

  return TRUE;
}

static TempBuf *
gimp_mypaint_brush_get_new_preview (GimpViewable *viewable,
                            GimpContext  *context,
                            gint          width,
                            gint          height)
{
  GimpMypaintBrush     *mypaint_brush       = GIMP_MYPAINT_BRUSH (viewable);
  cairo_surface_t      *icon_image = mypaint_brush->icon;
  ScopeGuard<cairo_surface_t, void(cairo_surface_t*)> scaled_icon(cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height), cairo_surface_destroy);
  
  TempBuf       *return_buf  = NULL;
  gint           icon_width = cairo_image_surface_get_width (icon_image);
  gint           icon_height = cairo_image_surface_get_height (icon_image);
  guchar         transp[4]   = { 0, 0, 0, 0 };
  gint           x, y;
  gdouble        scale = 1.0;

  ScopeGuard<cairo_t, void(cairo_t*)> cr(cairo_create(scaled_icon.ptr()), cairo_destroy);
  gimp_mypaint_brush_begin_use (mypaint_brush);
  if (icon_width > width || icon_height > height)
    {
      gdouble ratio_x = (gdouble) width  / (gdouble) icon_width;
      gdouble ratio_y = (gdouble) height / (gdouble) icon_height;
      scale   = MIN (ratio_x, ratio_y);
      cairo_scale (cr.ptr(), scale, scale);
    }
  cairo_set_source_surface (cr.ptr(), icon_image, 0, 0);
  cairo_paint (cr.ptr());
  
  return_buf = temp_buf_new (width, height, 4, 0, 0, transp);

  unsigned char *source_buf = cairo_image_surface_get_data (scaled_icon.ptr());
  guchar *dest_buf   = temp_buf_get_data (return_buf);
  gint stride       = cairo_image_surface_get_stride (scaled_icon.ptr());
  
  for (y = 0; y < height; y ++) 
    {
      for (x = 0; x < width; x ++)
        {
          dest_buf[x * 4    ] = source_buf[x * 4 + 2];
          dest_buf[x * 4 + 1] = source_buf[x * 4 + 1];
          dest_buf[x * 4 + 2] = source_buf[x * 4 + 0];
          dest_buf[x * 4 + 3] = source_buf[x * 4 + 3];
        }
      source_buf += stride;
      dest_buf += width * 4;
    }
  gimp_mypaint_brush_end_use (mypaint_brush);

  return return_buf;
}

static gchar *
gimp_mypaint_brush_get_description (GimpViewable  *viewable,
                            gchar        **tooltip)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (viewable);
  g_return_val_if_fail (mypaint_brush != NULL, NULL);

  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(mypaint_brush->p);  
  g_return_val_if_fail (priv != NULL, NULL);
  
  gchar *desc1 = priv->get_parent_brush_name();
  
  if (desc1[0] != '\0')
    return g_strdup (desc1);
    
  g_object_get (G_OBJECT (viewable), "name", &desc1, NULL);
  return g_strdup (desc1);
}

static void
gimp_mypaint_brush_dirty (GimpData *data)
{
  GimpMypaintBrush *mypaint_brush = GIMP_MYPAINT_BRUSH (data);
#if 0
  if (mypaint_brush->mask_cache)
    gimp_mypaint_brush_cache_clear (mypaint_brush->mask_cache);

  if (mypaint_brush->pixmap_cache)
    gimp_mypaint_brush_cache_clear (mypaint_brush->pixmap_cache);

  if (mypaint_brush->boundary_cache)
    gimp_mypaint_brush_cache_clear (mypaint_brush->boundary_cache);
#endif
  GIMP_DATA_CLASS (parent_class)->dirty (data);
}

static const gchar *
gimp_mypaint_brush_get_extension (GimpData *data)
{
  return GIMP_MYPAINT_BRUSH_FILE_EXTENSION;
}

static void
gimp_mypaint_brush_real_begin_use (GimpMypaintBrush *mypaint_brush)
{
}

static void
gimp_mypaint_brush_real_end_use (GimpMypaintBrush *mypaint_brush)
{
}

static GimpMypaintBrush *
gimp_mypaint_brush_real_select_mypaint_brush (GimpMypaintBrush        *mypaint_brush,
                              const GimpCoords *last_coords,
                              const GimpCoords *current_coords)
{
  return mypaint_brush;
}

static gboolean
gimp_mypaint_brush_real_want_null_motion (GimpMypaintBrush        *mypaint_brush,
                                  const GimpCoords *last_coords,
                                  const GimpCoords *current_coords)
{
  return TRUE;
}

static gchar *
gimp_mypaint_brush_get_checksum (GimpTagged *tagged)
{
  GimpMypaintBrush *mypaint_brush           = GIMP_MYPAINT_BRUSH (tagged);
  gchar     *checksum_string = NULL;
#if 0
  if (mypaint_brush->mask)
    {
      GChecksum *checksum = g_checksum_new (G_CHECKSUM_MD5);

      g_checksum_update (checksum, temp_buf_get_data (mypaint_brush->mask), temp_buf_get_data_size (mypaint_brush->mask));
      if (mypaint_brush->pixmap)
        g_checksum_update (checksum, temp_buf_get_data (mypaint_brush->pixmap), temp_buf_get_data_size (mypaint_brush->pixmap));
      g_checksum_update (checksum, (const guchar *) &mypaint_brush->spacing, sizeof (mypaint_brush->spacing));
      g_checksum_update (checksum, (const guchar *) &mypaint_brush->x_axis, sizeof (mypaint_brush->x_axis));
      g_checksum_update (checksum, (const guchar *) &mypaint_brush->y_axis, sizeof (mypaint_brush->y_axis));

      checksum_string = g_strdup (g_checksum_get_string (checksum));

      g_checksum_free (checksum);
    }
#endif
  return checksum_string;
}

/*  public functions  */

GimpData *
gimp_mypaint_brush_new (GimpContext *context,
                        const gchar *name)
{
  g_return_val_if_fail (name != NULL, NULL);
  g_return_val_if_fail (name[0] != '\0', NULL);

  GimpMypaintBrush *brush;
  brush = GIMP_MYPAINT_BRUSH (g_object_new (GIMP_TYPE_MYPAINT_BRUSH,
                                            "name", name,
                                            "mime-type", "application/x-mypaint-brush",
                                            NULL));

  return GIMP_DATA (brush);
}

GimpData *
gimp_mypaint_brush_get_standard (GimpContext *context)
{
  static GimpData *standard_mypaint_brush = NULL;

  if (! standard_mypaint_brush)
    {
      standard_mypaint_brush = gimp_mypaint_brush_new (context, "Standard");

      gimp_data_clean (standard_mypaint_brush);
      gimp_data_make_internal (standard_mypaint_brush, "gimp-mypaint_brush-standard");

      g_object_add_weak_pointer (G_OBJECT (standard_mypaint_brush),
                                 (gpointer *) &standard_mypaint_brush);
    }

  return standard_mypaint_brush;
}

void
gimp_mypaint_brush_begin_use (GimpMypaintBrush *mypaint_brush)
{
  g_return_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush));

  mypaint_brush->use_count++;

  if (mypaint_brush->use_count == 1)
    GIMP_MYPAINT_BRUSH_GET_CLASS (mypaint_brush)->begin_use (mypaint_brush);
}

void
gimp_mypaint_brush_end_use (GimpMypaintBrush *mypaint_brush)
{
  g_return_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush));
  g_return_if_fail (mypaint_brush->use_count > 0);

  mypaint_brush->use_count--;

  if (mypaint_brush->use_count == 0)
    GIMP_MYPAINT_BRUSH_GET_CLASS (mypaint_brush)->end_use (mypaint_brush);
}

GimpMypaintBrush *
gimp_mypaint_brush_select_mypaint_brush (GimpMypaintBrush        *mypaint_brush,
                         const GimpCoords *last_coords,
                         const GimpCoords *current_coords)
{
  g_return_val_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush), NULL);
  g_return_val_if_fail (last_coords != NULL, NULL);
  g_return_val_if_fail (current_coords != NULL, NULL);

  return GIMP_MYPAINT_BRUSH_GET_CLASS (mypaint_brush)->select_mypaint_brush (mypaint_brush,
                                                     last_coords,
                                                     current_coords);
}

gboolean
gimp_mypaint_brush_want_null_motion (GimpMypaintBrush        *mypaint_brush,
                             const GimpCoords *last_coords,
                             const GimpCoords *current_coords)
{
  g_return_val_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush), FALSE);
  g_return_val_if_fail (last_coords != NULL, FALSE);
  g_return_val_if_fail (current_coords != NULL, FALSE);

  return GIMP_MYPAINT_BRUSH_GET_CLASS (mypaint_brush)->want_null_motion (mypaint_brush,
                                                         last_coords,
                                                         current_coords);
}

#if 0
gint
gimp_mypaint_brush_get_spacing (const GimpMypaintBrush *mypaint_brush)
{
  g_return_val_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush), 0);

  return mypaint_brush->spacing;
}

void
gimp_mypaint_brush_set_spacing (GimpMypaintBrush *mypaint_brush,
                        gint       spacing)
{
  g_return_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush));

  if (mypaint_brush->spacing != spacing)
    {
      mypaint_brush->spacing = spacing;

      g_signal_emit (mypaint_brush, mypaint_brush_signals[SPACING_CHANGED], 0);
      g_object_notify (G_OBJECT (mypaint_brush), "spacing");
    }
}
#endif

gboolean
gimp_mypaint_brush_calculate           (const GimpMypaintBrush *mypaint_brush,
                                        gint    setting_index,
                                        gfloat *data, gfloat *result)
{
  g_return_val_if_fail (GIMP_IS_MYPAINT_BRUSH (mypaint_brush), FALSE);
  g_return_val_if_fail (mypaint_brush->p != NULL, FALSE);
  
  GimpMypaintBrushPrivate *priv = reinterpret_cast<GimpMypaintBrushPrivate*>(mypaint_brush->p);
  g_return_val_if_fail (priv != NULL, FALSE);
  
  g_return_val_if_fail (0 <= setting_index && setting_index < BRUSH_SETTINGS_COUNT, FALSE);
  
  GimpMypaintBrushPrivate::Value* setting = priv->get_setting(setting_index);
  
  if (setting->mapping)
    *result = setting->mapping->calculate(data);
  else
    *result = setting->base_value;

  return TRUE;
}

} /* extern C */



GimpMypaintBrushPrivate::GimpMypaintBrushPrivate() {
  parent_brush_name = g_strdup("");
  group = g_strdup("");
  for (int i = 0; i < BRUSH_SETTINGS_COUNT; i ++) {
    settings[i].base_value = 0;
    settings[i].mapping    = NULL;
  }
}

GimpMypaintBrushPrivate::~GimpMypaintBrushPrivate() {
  if (parent_brush_name) {
    g_free (parent_brush_name);
  }
  if (group) {
    g_free (group);
  }
  for (int i = 0; i < BRUSH_SETTINGS_COUNT; i ++) {
    if (settings[i].mapping) {
      delete settings[i].mapping;
      settings[i].mapping    = NULL;
    }
  }
}

void 
GimpMypaintBrushPrivate::set_base_value (int index, float value) {
  g_assert (index >= 0 && index < BRUSH_SETTINGS_COUNT);
  allocate_mapping(index);
  settings[index].mapping->base_value = value;
  /*
  settings[index].base_value = value;
  if (settings[index].mapping)
    settings[index].mapping->base_value = value;
  */
}

void 
GimpMypaintBrushPrivate::allocate_mapping (int index) {
  g_assert (index >= 0 && index < BRUSH_SETTINGS_COUNT);
  if (!settings[index].mapping) {
    settings[index].mapping = new Mapping(INPUT_COUNT);
  }
}

void
GimpMypaintBrushPrivate::deallocate_mapping (int index) {
  g_assert (index >= 0 && index < BRUSH_SETTINGS_COUNT);
  if (settings[index].mapping) {
    delete settings[index].mapping;
    settings[index].mapping = NULL;
  }
}

char*
GimpMypaintBrushPrivate::get_parent_brush_name() {
  return parent_brush_name;
}

void
GimpMypaintBrushPrivate::set_parent_brush_name(char *name) {
  g_assert (name != NULL);
  if (parent_brush_name)
    g_free (parent_brush_name);
  parent_brush_name = g_strdup(name);
}

char* 
GimpMypaintBrushPrivate::get_group() {
  return group;
}

void
GimpMypaintBrushPrivate::set_group(char *name) {
  g_assert (name != NULL);
  if (group)
    g_free (group);
  group = g_strdup(name);
}
