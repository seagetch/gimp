/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpCloneLayer
 * Copyright (C) 2009  Michael Natterer <mitch@gimp.org>
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

#include "base/delegators.hpp"
#include "base/glib-cxx-types.hpp"
#include "base/glib-cxx-impl.hpp"
#include "base/glib-cxx-utils.hpp"

extern "C" {
#include "config.h"

#include <string.h>

#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"

#include "core-types.h"
#include "core-enums.h"

#include "base/tile.h"
#include "base/tile-manager.h"
#include "base/pixel-region.h"

#include "gimp.h"
#include "pdb/gimppdb.h"

#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpitem.h"
#include "gimpdrawable-private.h" /* eek */
#include "gimpdrawablestack.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpcontext.h"
#include "gimpprogress.h"
#include "gimpcontainer.h"
#include "gimpgrouplayer.h"

#include "gimp-intl.h"
#include "gimplayer.h"
#include "gimplayermask.h"

#include "gegl/gimp-gegl-utils.h"
#include "paint-funcs/paint-funcs.h"

#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"
#include "core/gimpparamspecs.h"
#include "plug-in/gimptemporaryprocedure.h"
}

#include "gimpclonelayer.h"
#include "gimpclonelayerundo.h"


namespace GLib {

struct CloneLayer : virtual public ImplBase, virtual public CloneLayerInterface
{
  GimpLayer*       source_layer;

  /*  hackish temp states to make the projection/tiles stuff work  */
  gboolean         reallocate_projection;
  gint             reallocate_width;
  gint             reallocate_height;

  Delegators::Connection* parent_changed_conn;
  Delegators::Connection* reorder_conn;

  CloneLayer(GObject* o) : ImplBase(o) {}
  virtual ~CloneLayer();

  static void class_init(Traits<GimpCloneLayer>::Class* klass);
  template<typename IFaceClass>
  static void iface_init(IFaceClass* klass);

  // Inherited methods
  virtual void            init         ();
  virtual void            finalize     ();
  virtual void            constructed  ();
  virtual void            set_property (guint            property_id,
                                        const GValue    *value,
                                        GParamSpec      *pspec);
  virtual void            get_property (guint            property_id,
                                        GValue          *value,
                                        GParamSpec      *pspec);

  virtual gint64          get_memsize  (gint64          *gui_size);

  virtual gboolean        get_size     (gint            *width,
                                        gint            *height);

  virtual GimpItem      * duplicate    (GType            new_type);
  virtual void            convert      (GimpImage       *dest_image);
  virtual void            translate    (gint             offset_x,
                                        gint             offset_y,
                                        gboolean         push_undo);
  virtual void            scale        (gint             new_width,
                                        gint             new_height,
                                        gint             new_offset_x,
                                        gint             new_offset_y,
                                        GimpInterpolationType  interp_type,
                                        GimpProgress    *progress);
  virtual void            resize       (GimpContext     *context,
                                        gint             new_width,
                                        gint             new_height,
                                        gint             offset_x,
                                        gint             offset_y);
  virtual void            flip         (GimpContext     *context,
                                        GimpOrientationType flip_type,
                                        gdouble          axis,
                                        gboolean         clip_result);
  virtual void            rotate       (GimpContext     *context,
                                        GimpRotationType rotate_type,
                                        gdouble          center_x,
                                        gdouble          center_y,
                                        gboolean         clip_result);
  virtual void            transform    (GimpContext     *context,
                                        const GimpMatrix3 *matrix,
                                        GimpTransformDirection direction,
                                        GimpInterpolationType  interpolation_type,
                                        gint             recursion_level,
                                        GimpTransformResize clip_result,
                                        GimpProgress    *progress);

  virtual gint64      estimate_memsize (gint             width,
                                        gint             height);

  virtual gint         get_opacity_at  (gint             x,
                                        gint             y);

  virtual void            update       ();
  virtual void            update_size  ();

  virtual void            set_source   (GimpLayer* source);
  virtual GimpLayer*      get_source   ();

  virtual gboolean        is_editable  ();

  // Event handlers
  virtual void       on_parent_changed (GimpViewable  *viewable,
                                        GimpViewable  *parent);

  virtual void        on_source_update (GimpDrawable* drawable,
                                        gint          x,
                                        gint          y,
                                        gint          width,
                                        gint          height);
  virtual void        on_reorder       (GimpContainer     *container,
                                        GimpLayer         *layer,
                                        gint               index);
private:
  void                invalidate_area  (gint               x,
                                        gint               y,
                                        gint               width,
                                        gint               height);
  void                invalidate_layer ();
};


extern const char gimp_clone_layer_name[] = "GimpCloneLayer";
using Class = GClass<gimp_clone_layer_name,
                     UseCStructs<GimpLayer, GimpCloneLayer>,
                     CloneLayer, GimpPickable>;

static GimpUndo *
gimp_image_undo_push_clone_layer_convert (GimpImage      *image,
                                          const gchar    *undo_desc,
                                          GimpCloneLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (Class::Traits::is_instance(group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, Traits<GimpCloneLayerUndo>::get_type(),
                               GIMP_UNDO_GROUP_LAYER_CONVERT, undo_desc,
                               GimpDirtyMask(GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE),
                               "item", group,
                               NULL);
}


#define bind_to_class(klass, method, impl)  Class::__(&klass->method).bind<&impl::method>()

void CloneLayer::class_init(Traits<GimpCloneLayer>::Class *klass)
{
  typedef CloneLayer Impl;
  g_print("CloneLayer::class_init\n");
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  bind_to_class (object_class,      set_property,     Impl);
  bind_to_class (object_class,      get_property,     Impl);
  bind_to_class (object_class,      finalize,         Impl);
  bind_to_class (object_class,      constructed,      Impl);
  bind_to_class (gimp_object_class, get_memsize,      Impl);

  viewable_class->default_stock_id = "gtk-duplicate";
  bind_to_class (viewable_class,    get_size,         Impl);
  bind_to_class (item_class,        duplicate,        Impl);
  bind_to_class (item_class,        translate,        Impl);
  bind_to_class (item_class,        scale,            Impl);
  bind_to_class (item_class,        resize,           Impl);
  bind_to_class (item_class,        flip,             Impl);
  bind_to_class (item_class,        rotate,           Impl);
  bind_to_class (item_class,        transform,        Impl);
//bind_to_class (item_class,        convert,          Impl);
  bind_to_class (item_class,        is_editable,      Impl);
  bind_to_class (drawable_class,    estimate_memsize, Impl);

  item_class->default_name         = _("Clone Layer");
  item_class->rename_desc          = C_("undo-type", "Rename Clone Layer");
  item_class->translate_desc       = C_("undo-type", "Move Clone Layer");
  item_class->scale_desc           = C_("undo-type", "Scale Clone Layer");
  item_class->resize_desc          = C_("undo-type", "Resize Clone Layer");
  item_class->flip_desc            = C_("undo-type", "Flip Clone Layer");
  item_class->rotate_desc          = C_("undo-type", "Rotate Clone Layer");
  item_class->transform_desc       = C_("undo-type", "Transform Clone Layer");

  ImplBase::class_init<Traits<GimpCloneLayer>::Class, CloneLayer>(klass);
}

template<>
void CloneLayer::iface_init<GimpPickableInterface>(GimpPickableInterface* iface)
{
  typedef CloneLayer Impl;
  g_print("CloneLayer::iface_init<GimpPickableInterface>\n");
  
  bind_to_class (iface, get_opacity_at, Impl);
}

}; // namespace

GLib::CloneLayer::~CloneLayer()
{
  g_print("~CloneLayer\n");
}

void GLib::CloneLayer::init ()
{
  parent_changed_conn = g_signal_connect_delegator (G_OBJECT(g_object), "parent-changed",
                                                    Delegators::delegator(this, &GLib::CloneLayer::on_parent_changed));
  reorder_conn        = NULL;
}

void GLib::CloneLayer::constructed ()
{
  on_parent_changed(GIMP_VIEWABLE(g_object), NULL);
}

void GLib::CloneLayer::finalize ()
{
  g_print("CloneLayer::finalize\n");
  if (reorder_conn) {
    delete reorder_conn;
    reorder_conn = NULL;
  }

  G_OBJECT_CLASS (Class::parent_class)->finalize (g_object);
}

void GLib::CloneLayer::set_property (guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
    break;
  }
}

void GLib::CloneLayer::get_property (guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
    break;
  }
}

void GLib::CloneLayer::set_source(GimpLayer* layer)
{

  g_signal_connect_delegator (G_OBJECT(layer), "update",
                              Delegators::delegator(this, &GLib::CloneLayer::on_source_update));
  auto src  = _G(layer);
  gint w, h;
  w = src [gimp_item_get_width] ();
  h = src [gimp_item_get_height] ();
  auto self = _G(g_object);
  self [gimp_item_set_size] (w, h);
  gint width  = self [gimp_item_get_width]  ();
  gint height = self [gimp_item_get_height] ();

  on_source_update(GIMP_DRAWABLE(layer), 0, 0, width, height);
}

GimpLayer*
GLib::CloneLayer::get_source()
{

}

gint64 GLib::CloneLayer::get_memsize (gint64     *gui_size)
{
  gint64                 memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (Class::parent_class)->get_memsize (GIMP_OBJECT(g_object),
                                                                  gui_size);
}

gboolean GLib::CloneLayer::get_size (gint         *width,
                                     gint         *height)
{
  if (reallocate_width  != 0 &&
      reallocate_height != 0) {
    nullable(width)  = reallocate_width;
    nullable(height) = reallocate_height;
    return TRUE;
  }

  gboolean result = GIMP_VIEWABLE_CLASS (Class::parent_class)->get_size (GIMP_VIEWABLE(g_object), width, height);
  return result;
}

GimpItem* GLib::CloneLayer::duplicate (GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (Class::parent_class)->duplicate (GIMP_ITEM(g_object), new_type);
  if (CloneLayerInterface::is_instance(new_item)) {
    dynamic_cast<CloneLayer*>(CloneLayerInterface::cast(new_item))->set_source(source_layer);
  }
  return new_item;
}

void GLib::CloneLayer::convert (GimpImage *dest_image)
{
  GIMP_ITEM_CLASS (Class::parent_class)->convert (GIMP_ITEM(g_object), dest_image);
}

void GLib::CloneLayer::translate (gint      offset_x,
                                  gint      offset_y,
                                  gboolean  push_undo)
{
  GIMP_ITEM_CLASS(Class::parent_class)->translate(GIMP_ITEM(g_object), offset_x, offset_y, push_undo);
//  invalidate_layer();
}

void GLib::CloneLayer::scale (gint                   new_width,
                                 gint                   new_height,
                                 gint                   new_offset_x,
                                 gint                   new_offset_y,
                                 GimpInterpolationType  interpolation_type,
                                 GimpProgress          *progress)
{
  GIMP_ITEM_CLASS(Class::parent_class)->scale(GIMP_ITEM(g_object), new_width, new_height, new_offset_x, new_offset_y, interpolation_type, progress);
//  invalidate_layer();
}

void GLib::CloneLayer::resize (GimpContext *context,
                               gint         new_width,
                               gint         new_height,
                               gint         offset_x,
                               gint         offset_y)
{
  GIMP_ITEM_CLASS(Class::parent_class)->resize(GIMP_ITEM(g_object), context, new_width, new_height, offset_x, offset_y);
//  invalidate_layer();
}

void GLib::CloneLayer::flip (GimpContext         *context,
                             GimpOrientationType  flip_type,
                             gdouble              axis,
                             gboolean             clip_result)
{
  GIMP_ITEM_CLASS(Class::parent_class)->flip(GIMP_ITEM(g_object), context, flip_type, axis, clip_result);
//  invalidate_layer();
}

void GLib::CloneLayer::rotate (GimpContext      *context,
                                  GimpRotationType  rotate_type,
                                  gdouble           center_x,
                                  gdouble           center_y,
                                  gboolean          clip_result)
{
  GIMP_ITEM_CLASS(Class::parent_class)->rotate(GIMP_ITEM(g_object), context, rotate_type, center_x, center_y, clip_result);
//  invalidate_layer();
}

void GLib::CloneLayer::transform (GimpContext            *context,
                                     const GimpMatrix3      *matrix,
                                     GimpTransformDirection  direction,
                                     GimpInterpolationType   interpolation_type,
                                     gint                    recursion_level,
                                     GimpTransformResize     clip_result,
                                     GimpProgress           *progress)
{
  GIMP_ITEM_CLASS(Class::parent_class)->transform(GIMP_ITEM(g_object), context, matrix, direction, interpolation_type, recursion_level, clip_result, progress);
//  invalidate_layer();
}

gint64 GLib::CloneLayer::estimate_memsize (gint                width,
                                           gint                height)
{
  return GIMP_DRAWABLE_CLASS (Class::parent_class)->estimate_memsize (GIMP_DRAWABLE(g_object),
                                                                      width,
                                                                      height);
}

gint GLib::CloneLayer::get_opacity_at (gint          x,
                                       gint          y)
{
  /* Only consider child layers as having content */
  return 0;
}


/*  priv functions  */

void GLib::CloneLayer::update () {
  update_size ();
}

void GLib::CloneLayer::update_size () {
  auto self       = _G(g_object);
  gint     old_x      = self [gimp_item_get_offset_x] ();
  gint     old_y      = self [gimp_item_get_offset_y] ();
  gint     old_width  = self [gimp_item_get_width]  ();
  gint     old_height = self [gimp_item_get_height] ();
  gint     x          = 0;
  gint     y          = 0;
  gint     width      = 1;
  gint     height     = 1;

  if (!source_layer)
    return;
  auto src = _G(source_layer);
  src [gimp_item_get_offset] (&x, &y);
  width  = src [gimp_item_get_width] ();
  height = src [gimp_item_get_height] ();

  g_print ("%s (%s) %d, %d (%d, %d)\n",
           G_STRFUNC, gimp_object_get_name (g_object),
           x, y, width, height);

  if (reallocate_projection ||
      x      != old_x       ||
      y      != old_y       ||
      width  != old_width   ||
      height != old_height) {

    auto self = _G(g_object);
    if (reallocate_projection ||
        width  != old_width   ||
        height != old_height) {
      reallocate_projection = FALSE;

      reallocate_width  = width;
      reallocate_height = height;

      g_print("set_size:%d,%d\n", width, height);
      resize(NULL, width, height, x, y);

      self [gimp_pickable_flush] ();
//      self [gimp_projectable_structure_changed] ();
    }
  }
}

gboolean GLib::CloneLayer::is_editable()
{
  return FALSE;
}

void GLib::CloneLayer::invalidate_area (gint               x,
                                         gint               y,
                                         gint               width,
                                         gint               height)
{
//  g_print("%s: invalidate_area(%d, %d  -  %d,%d)\n", _G(g_object)[gimp_object_get_name](), x, y, width, height);
  gint parent_off_x = 0;
  gint parent_off_y = 0;
  GimpViewable* parent = gimp_viewable_get_parent(GIMP_VIEWABLE(g_object));
  if (parent)
    gimp_item_get_offset(GIMP_ITEM(parent), &parent_off_x, &parent_off_y);
  GimpItem* self_item = GIMP_ITEM(g_object);
  GimpImage* image = gimp_item_get_image(self_item);
  gint item_width  = gimp_item_get_width (self_item);
  gint item_height = gimp_item_get_height(self_item);
  gint offset_x    = gimp_item_get_offset_x (self_item);
  gint offset_y    = gimp_item_get_offset_y (self_item);
  gint image_width = gimp_image_get_width(image);
  gint image_height= gimp_image_get_height(image);

  gint       x1           = x - parent_off_x,
             x2           = x1 + width,
             y1           = y - parent_off_y,
             y2           = y1 + height;
  if (x2 < offset_x - parent_off_x ||
      y2 < offset_y - parent_off_y ||
      x1 > offset_x - parent_off_x + item_width ||
      y1 > offset_y - parent_off_y + item_height )
    return;

  x1 = MAX((x1 / TILE_WIDTH ) * TILE_WIDTH,  offset_x - parent_off_x);
  y1 = MAX((y1 / TILE_HEIGHT) * TILE_HEIGHT, offset_y - parent_off_y);
  x2 = MIN(image_width  - parent_off_x,
           MIN(offset_x - parent_off_x + item_width,
               ((x2 + TILE_WIDTH  - 1) / TILE_WIDTH ) * TILE_WIDTH ));
  y2 = MIN(image_height - parent_off_y,
           MIN(offset_y - parent_off_y + item_height,
               ((y2 + TILE_HEIGHT - 1) / TILE_HEIGHT) * TILE_HEIGHT));
}

void GLib::CloneLayer::invalidate_layer ()
{
  auto self         = _G(g_object);
  auto image        = _G( self [gimp_item_get_image] () );
  gint width        = self [gimp_item_get_width] ();
  gint height       = self [gimp_item_get_height] ();
  gint offset_x     = self [gimp_item_get_offset_x] ();
  gint offset_y     = self [gimp_item_get_offset_y] ();

  invalidate_area(offset_x, offset_y, width, height);

  GimpViewable*  parent = gimp_viewable_get_parent(GIMP_VIEWABLE(g_object));
  if (parent) {
    auto proj = _G( GIMP_PROJECTABLE(parent) );
    proj [gimp_projectable_invalidate] (offset_x, offset_y, width, height);
    proj [gimp_projectable_flush] (TRUE);
  }

  image [gimp_image_invalidate] (offset_x, offset_y, width, height, 0);
  auto projection = _G(image [gimp_image_get_projection] ());
  projection [gimp_projection_flush_now] ();
  image [gimp_image_flush] ();
}


void GLib::CloneLayer::on_parent_changed (GimpViewable  *viewable,
                                             GimpViewable  *parent)
{
//  g_print("on_parent_changed: %p ==> parent=%s\n", viewable, parent? gimp_object_get_name(parent): NULL);
  // Check for loop
}

void GLib::CloneLayer::on_source_update (GimpDrawable* _source,
                                         gint            x,
                                         gint            y,
                                         gint            width,
                                         gint            height)
{
  g_print ("%s (%s) %d, %d (%d, %d)\n",
           G_STRFUNC, gimp_object_get_name (g_object),
           x, y, width, height);

  update_size();

  auto self   = _G(g_object);
  auto source = _G(_source);

  /*  the projection speaks in image coordinates, transform to layer
   *  coordinates when emitting our own update signal.
   */
  gint offset_x = self [gimp_item_get_offset_x]();
  gint offset_y = self [gimp_item_get_offset_y]();

  source_layer = source;

  PixelRegion destPR;
  TileManager* dest_tiles = self [gimp_drawable_get_tiles] ();
  pixel_region_init (&destPR, dest_tiles, x, y, width, height, TRUE);

  GimpImageType self_type   = self [gimp_drawable_type] ();
  GimpImageType source_type = source [gimp_drawable_type] ();

//  if (self_type == source_type) {
//    PixelRegion srcPR;
//    TileManager* src_tiles  = source [gimp_drawable_get_tiles] ();
//    pixel_region_init (&srcPR,  src_tiles,  x, y, width, height, FALSE);
//
//    copy_region_nocow(&srcPR, &destPR);

//  } else {
    source [gimp_drawable_project_region] (x, y, width, height, &destPR, FALSE);
//  }

  self [gimp_drawable_update] (x, y, width, height);
}

void GLib::CloneLayer::on_reorder (GimpContainer* _container,
                                    GimpLayer* _layer,
                                    gint index)
{
//  g_print("%s,%d: CloneLayer::on_reorder(%s,%d)\n", _G(g_object)[gimp_object_get_name](), last_index, _G(_layer)[gimp_object_get_name](), index);
  auto layer = _G(_layer);
  auto stack = _G(_container);
  gint item_index = stack [gimp_container_get_child_index] (GIMP_OBJECT(_layer));
  gint self_index = stack [gimp_container_get_child_index] (GIMP_OBJECT(g_object));

  if (G_OBJECT(_layer) == g_object) {
  }
}

/*  public functions  */
GimpLayer *
CloneLayerInterface::new_instance (GimpImage            *image,
                                   GimpLayer            *source,
                                   const gchar          *name,
                                   gdouble               opacity,
                                   GimpLayerModeEffects  mode)
{
  GimpImageType   type;
  GimpCloneLayer* cloned;
  gint x, y, width, height;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  type = gimp_image_base_type_with_alpha (image);

  if (source) {
    auto src = GLib::_G(source);
    width  = src [gimp_item_get_width] ();
    height = src [gimp_item_get_height] ();
    src [gimp_item_get_offset] (&x, &x);
  } else {
    auto img = GLib::_G(image);
    width  = img [gimp_image_get_width] ();
    height = img [gimp_image_get_height] ();
    x = y = 0;
  }

  g_print("create clone instance type=%lx\n", GLib::Class::Traits::get_type());
  cloned = GLib::Class::Traits::cast (gimp_drawable_new (GLib::Class::Traits::get_type(),
                                                         image, name,
                                                         x, y, width, height,
                                                         type));
  GimpLayer* layer = GIMP_LAYER(cloned);
  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  CloneLayerInterface::cast(cloned)->set_source(source);

  g_print("new_instance::cloned=%p[layer=%p]\n", cloned, layer);

  return layer;
}

CloneLayerInterface* CloneLayerInterface::cast(gpointer obj)
{
  if (!is_instance(obj))
    return NULL;
  return dynamic_cast<CloneLayerInterface*>(GLib::Class::get_private(obj));
}

bool CloneLayerInterface::is_instance(gpointer obj)
{
  return GLib::Class::Traits::is_instance(obj);
}

GType gimp_clone_layer_get_type() {
  return GLib::Class::Traits::get_type();
}

GimpLayer*
gimp_clone_layer_new            (GimpImage            *image,
                                 GimpLayer            *source,
                                  const gchar          *name,
                                  gdouble               opacity,
                                  GimpLayerModeEffects  mode)
{
  return CloneLayerInterface::new_instance(image, source, name, opacity, mode);
}

void
gimp_clone_layer_set_source    (GimpCloneLayer* layer, GimpLayer* source)
{
  if (CloneLayerInterface::is_instance(layer))
    CloneLayerInterface::cast(layer)->set_source(source);
}

GimpLayer*
gimp_clone_layer_set_source    (GimpCloneLayer* layer)
{
  if (CloneLayerInterface::is_instance(layer))
    return CloneLayerInterface::cast(layer)->get_source();
  return NULL;
}
