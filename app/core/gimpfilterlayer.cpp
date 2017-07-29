/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpFilterLayer
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
#include "gimpdrawable-private.h" /* eek */
#include "gimpdrawablestack.h"
#include "gimppickable.h"
#include "gimpprojectable.h"
#include "gimpprojection.h"
#include "gimpcontext.h"
#include "gimpprogress.h"
#include "gimpcontainer.h"

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

#include "gimpfilterlayer.h"
#include "gimpfilterlayerundo.h"


namespace GLib {
class ProcedureRunner {
  GimpProcedure* procedure;
  GimpProgress*  progress;
  GValueArray*   args;
  bool           running;

public:
  ProcedureRunner(GimpProcedure* proc, GimpProgress* prog) :
    procedure(proc), progress(prog), running(false), args(NULL) {};
  ~ProcedureRunner() {
    if (running)
      gimp_progress_cancel(progress);
    if (args) {
      g_value_array_free(args);
      args = NULL;
    }
  }

  bool is_running() { return running; };

  bool setup_args(HashTable<const gchar*, GValue*>* arg_tables) {
    if (!args)
      args = gimp_procedure_get_arguments (procedure);

    if (!arg_tables)
      return true;
    for (int i = 0; i < procedure->num_args; i ++) {
      GParamSpec* pspec = procedure->args[i];
      GValue* value = (arg_tables)? (*arg_tables)[g_param_spec_get_name(pspec)] : NULL;
      if (value) {
        g_value_copy(value, &args->values[i]);
      } else {
        GValue v_str = G_VALUE_INIT;
        g_value_init(&v_str, G_TYPE_STRING);
        g_value_transform(g_param_spec_get_default_value(pspec), &v_str);
        g_print("FilterLayer::setup_args: %s: %s = %s\n", g_param_spec_get_name(pspec), g_type_name(G_PARAM_SPEC_VALUE_TYPE(pspec)), g_value_get_string(&v_str) );
        g_value_copy(g_param_spec_get_default_value(pspec), &args->values[i]);
      }
    }
    return true;
  }

  bool run(GimpItem* item) {
    if (running)
      return false;
    GimpDrawable*  drawable= GIMP_DRAWABLE(item);
    GimpImage*     image   = gimp_item_get_image(GIMP_ITEM(drawable));
    Gimp*          gimp    = image->gimp;
    GimpPDB*       pdb     = gimp->pdb;
    GimpContext*   context = gimp_get_user_context(gimp);
    GError*        error   = NULL;
    GimpObject*    display = GIMP_OBJECT(gimp_context_get_display(context));
    gint           n_args  = 0;

    if (!procedure)
      return false;

    if (!args)
      setup_args(NULL);

    for (int i = 0; i < procedure->num_args; i ++) {

      GParamSpec* pspec = procedure->args[i];
      if (strcmp(g_param_spec_get_name(pspec),"run-mode") == 0) {
        g_value_set_int(&args->values[i], GIMP_RUN_NONINTERACTIVE);

      } else if (GIMP_IS_PARAM_SPEC_IMAGE_ID(pspec)) {
        gimp_value_set_image (&args->values[i], image);

      } else if (GIMP_IS_PARAM_SPEC_DRAWABLE_ID(pspec)) {
        gimp_value_set_drawable (&args->values[i], drawable);

      } else if (GIMP_IS_PARAM_SPEC_ITEM_ID(pspec)) {
        gimp_value_set_item (&args->values[i], item);

      }

    }

    gimp_procedure_execute_async (procedure, gimp, context,
                                  progress, args, display,NULL);
    gimp_image_flush (image);
    running = true;
    return true;
  }

  void on_end() {
    running = false;
  }

  const char* get_procedure_name() {
    return (procedure)? procedure->original_name : "";
  }

  GValue get_arg(int index) {
    if (!procedure || index >= procedure->num_args)
      return G_VALUE_INIT;
    setup_args(NULL);
    GValue b;
    g_value_init (&b, G_TYPE_STRING);
    g_value_transform (&args->values[index], &b);
    g_print("get_arg(%d):%s\n", index, g_value_get_string(&b));
    return args->values[index];
  }

  GValueArray* get_args() {
    setup_args(NULL);
    return g_value_array_copy(args);
  }

  void set_arg(int index, GValue value) {
    if (!procedure || index >= procedure->num_args)
      return;
    setup_args(NULL);
    g_value_transform(&value, &args->values[index]);
  }

};

struct FilterLayer : virtual public ImplBase, virtual public FilterLayerInterface
{
  TileManager*     projected_tiles;
  bool             projected_tiles_updated;
  gdouble          filter_progress;
  ProcedureRunner* runner;
  ProcedureRunner* new_runner;
  GList*           updates;

  gint             suspend_resize_;

  /*  hackish temp states to make the projection/tiles stuff work  */
  gboolean         reallocate_projection;
  gint             reallocate_width;
  gint             reallocate_height;

  struct Rectangle {
    gint x, y;
    gint width, height;
    Rectangle(gint x_, gint y_, gint w_, gint h_) : x(x_), y(y_), width(w_), height(h_) {};
  };

  Delegators::Connection* child_update_conn;
  Delegators::Connection* parent_changed_conn;

  FilterLayer(GObject* o) : ImplBase(o) {}
  virtual ~FilterLayer();

  static void class_init(Traits<GimpFilterLayer>::Class* klass);
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

  virtual void         project_region (gint          x,
                                       gint          y,
                                       gint          width,
                                       gint          height,
                                       PixelRegion  *projPR,
                                       gboolean      combine);
  virtual gint         get_opacity_at (gint             x,
                                       gint             y);

  virtual void            filter_reset ();
  virtual void            update       ();
  virtual void            update_size  ();

  virtual void            set_procedure(const char* proc_name, GValueArray* args = NULL);
  virtual const char*     get_procedure();
  virtual GValueArray*    get_procedure_args();
  virtual GValue          get_procedure_arg(int index);
  virtual void            set_procedure_arg(int index, GValue value);

  // For GimpProgress Interface
  virtual GimpProgress*   start        (const gchar* message,
                                        gboolean     cancelable);
  virtual void            end          ();
  virtual gboolean        is_active    ();
  virtual void            set_text     (const gchar* message);
  virtual void            set_value    (gdouble      percentage);
  virtual gdouble         get_value    ();
  virtual void            pulse        ();
  virtual gboolean        message      (Gimp*                gimp,
                                        GimpMessageSeverity severity,
                                        const gchar*        domain,
                                        const gchar*       message);

  // Event handlers
  virtual void       on_parent_changed (GimpViewable  *viewable,
                                        GimpViewable  *parent);

  virtual void         on_stack_update (GimpDrawableStack *stack,
                                        gint               x,
                                        gint               y,
                                        gint               width,
                                        gint               height,
                                        GimpItem          *item);
  virtual void         on_proj_update  (GimpProjection    *proj,
                                        gboolean           now,
                                        gint               x,
                                        gint               y,
                                        gint               width,
                                        gint               height);
  // Public interfaces
  virtual void             suspend_resize (gboolean        push_undo);
  virtual void             resume_resize  (gboolean        push_undo);

private:
  void                invalidate_area  (gint               x,
                                        gint               y,
                                        gint               width,
                                        gint               height);
  void                invalidate_layer ();
};


extern const char gimp_filter_layer_name[] = "GimpFilterLayer";
using Class = GClass<gimp_filter_layer_name,
                     UseCStructs<GimpLayer, GimpFilterLayer>,
                     FilterLayer,
                     GimpProgress, GimpPickable>;

static GimpUndo *
gimp_image_undo_push_filter_layer_suspend (GimpImage      *image,
                                           const gchar    *undo_desc,
                                           GimpFilterLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (Class::Traits::is_instance(group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, Traits<GimpFilterLayerUndo>::get_type(),
                               GIMP_UNDO_GROUP_LAYER_SUSPEND, undo_desc,
                               GimpDirtyMask(GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE),
                               "item",  group,
                               NULL);
}

static GimpUndo *
gimp_image_undo_push_filter_layer_resume (GimpImage      *image,
                                         const gchar    *undo_desc,
                                         GimpFilterLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (Class::Traits::is_instance(group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, Traits<GimpFilterLayerUndo>::get_type(),
                               GIMP_UNDO_GROUP_LAYER_RESUME, undo_desc,
                               GimpDirtyMask(GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE),
                               "item",  group,
                               NULL);
}

static GimpUndo *
gimp_image_undo_push_filter_layer_convert (GimpImage      *image,
                                          const gchar    *undo_desc,
                                          GimpFilterLayer *group)
{
  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);
  g_return_val_if_fail (Class::Traits::is_instance(group), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (group)), NULL);

  return gimp_image_undo_push (image, Traits<GimpFilterLayerUndo>::get_type(),
                               GIMP_UNDO_GROUP_LAYER_CONVERT, undo_desc,
                               GimpDirtyMask(GIMP_DIRTY_ITEM | GIMP_DIRTY_DRAWABLE),
                               "item", group,
                               NULL);
}


#define bind_to_class(klass, method, impl)  Class::__(&klass->method).bind<&impl::method>()

void FilterLayer::class_init(Traits<GimpFilterLayer>::Class *klass)
{
  typedef FilterLayer Impl;
  g_print("FilterLayer::class_init\n");
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  GimpObjectClass   *gimp_object_class = GIMP_OBJECT_CLASS (klass);
  GimpViewableClass *viewable_class    = GIMP_VIEWABLE_CLASS (klass);
  GimpItemClass     *item_class        = GIMP_ITEM_CLASS (klass);
  GimpDrawableClass *drawable_class    = GIMP_DRAWABLE_CLASS (klass);

  bind_to_class (object_class, set_property, Impl);
  bind_to_class (object_class, get_property, Impl);
  bind_to_class (object_class, finalize, Impl);
  bind_to_class (object_class, constructed, Impl);
  bind_to_class (gimp_object_class, get_memsize, Impl);

  viewable_class->default_stock_id = "gtk-directory";
  bind_to_class (viewable_class, get_size, Impl);

  bind_to_class (item_class, duplicate, Impl);
  bind_to_class (item_class, translate, Impl);
  bind_to_class (item_class, scale, Impl);
  bind_to_class (item_class, resize, Impl);
  bind_to_class (item_class, flip, Impl);
  bind_to_class (item_class, rotate, Impl);
  bind_to_class (item_class, transform, Impl);
  bind_to_class (drawable_class, project_region, Impl);
  bind_to_class (drawable_class, estimate_memsize, Impl);

  item_class->default_name         = _("Filter Layer");
  item_class->rename_desc          = C_("undo-type", "Rename Filter Layer");
  item_class->translate_desc       = C_("undo-type", "Move Filter Layer");
  item_class->scale_desc           = C_("undo-type", "Scale Filter Layer");
  item_class->resize_desc          = C_("undo-type", "Resize Filter Layer");
  item_class->flip_desc            = C_("undo-type", "Flip Filter Layer");
  item_class->rotate_desc          = C_("undo-type", "Rotate Filter Layer");
  item_class->transform_desc       = C_("undo-type", "Transform Filter Layer");
  //  F::__(&item_class->convert            ).bind<&Impl::convert>();

  ImplBase::class_init<Traits<GimpFilterLayer>::Class, FilterLayer>(klass);
}

template<>
void FilterLayer::iface_init<GimpPickableInterface>(GimpPickableInterface* iface)
{
  typedef FilterLayer Impl;
  g_print("FilterLayer::iface_init<GimpPickableInterface>\n");
  
  Class::__(&iface->get_opacity_at).bind<&Impl::get_opacity_at>();
}

template<>
void FilterLayer::iface_init<GimpProgressInterface>(GimpProgressInterface* iface)
{
  typedef GLib::FilterLayer Impl;
  bind_to_class (iface, start, Impl);
  bind_to_class (iface, end, Impl);
  bind_to_class (iface, is_active, Impl);
  bind_to_class (iface, set_text, Impl);
  bind_to_class (iface, set_value, Impl);
  bind_to_class (iface, get_value, Impl);
  bind_to_class (iface, pulse, Impl);
  bind_to_class (iface, message, Impl);
}

}; // namespace

GLib::FilterLayer::~FilterLayer()
{
}

void GLib::FilterLayer::init ()
{
  typedef Class F;
#if 0
  projection = gimp_projection_new (GIMP_PROJECTABLE (g_object));

  g_signal_connect_delegator (G_OBJECT(projection), "update",
                              Delegator::delegator(this, &GLib::FilterLayer::on_proj_update));
#endif
  child_update_conn   = NULL;
  parent_changed_conn = g_signal_connect_delegator (G_OBJECT(g_object), "parent-changed",
                                                    Delegators::delegator(this, &GLib::FilterLayer::on_parent_changed));
  updates             = NULL;
  projected_tiles     = NULL;
  filter_progress     = 0;
  runner              = NULL;
  projected_tiles_updated = false;
}

void GLib::FilterLayer::constructed ()
{
  g_print("FilterLayer::constructed: project_tiles = %p\n", projected_tiles);
  on_parent_changed(GIMP_VIEWABLE(g_object), NULL);

  // Setup ProcedureRunner object
  set_procedure("plug-in-edge");
  set_procedure("plug-in-gauss");
  HashTable<const gchar*, GValue*> args = g_hash_table_new(g_str_hash, g_str_equal);
  auto horizontal = Value(25.0);  args.insert("horizontal", &horizontal.ref());
  auto vertical   = Value(25.0);  args.insert("vertical",   &vertical.ref());
  runner->setup_args(&args);
}

void GLib::FilterLayer::finalize ()
{
  if (child_update_conn) {
    delete child_update_conn;
    child_update_conn = NULL;
  }

  if (projected_tiles) {
    tile_manager_unref(projected_tiles);
    projected_tiles = NULL;
  }

  if (runner) {
    delete runner;
    runner = NULL;
  }

  G_OBJECT_CLASS (Class::parent_class)->finalize (g_object);
}

void GLib::FilterLayer::set_property (guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
    break;
  }
}

void GLib::FilterLayer::get_property (guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
    break;
  }
}

gint64 GLib::FilterLayer::get_memsize (gint64     *gui_size)
{
  gint64                 memsize = 0;

  return memsize + GIMP_OBJECT_CLASS (Class::parent_class)->get_memsize (GIMP_OBJECT(g_object),
                                                                  gui_size);
}

gboolean GLib::FilterLayer::get_size (gint         *width,
                                        gint         *height)
{
  if (reallocate_width  != 0 &&
      reallocate_height != 0)
    {
      *width  = reallocate_width;
      *height = reallocate_height;

      g_print("w,h=%d,%d",*width, *height);
      return TRUE;
    }

  gboolean result = GIMP_VIEWABLE_CLASS (Class::parent_class)->get_size (GIMP_VIEWABLE(g_object), width, height);
  g_print("inherited: w,h=%d,%d",*width, *height);
  return result;
}

GimpItem* GLib::FilterLayer::duplicate (GType     new_type)
{
  GimpItem *new_item;

  g_return_val_if_fail (g_type_is_a (new_type, GIMP_TYPE_DRAWABLE), NULL);

  new_item = GIMP_ITEM_CLASS (Class::parent_class)->duplicate (GIMP_ITEM(g_object), new_type);

  if (GIMP_IS_FILTER_LAYER (new_item))
    {
      GLib::FilterLayer *new_priv = Class::get_private(new_item);
      gint                   position    = 0;
      GList                 *list;

      new_priv->suspend_resize (FALSE);

      /*  force the projection to reallocate itself  */
      new_priv->reallocate_projection = TRUE;

      new_priv->resume_resize (FALSE);
    }

  return new_item;
}

void GLib::FilterLayer::convert (GimpImage *dest_image)
{
  GList                 *list;
#if 0
  for (list = gimp_item_stack_get_item_iter (GIMP_ITEM_STACK (children));
       list;
       list = g_list_next (list))
    {
      GimpItem *child = GIMP_ITEM(list->data);

      GIMP_ITEM_GET_CLASS (child)->convert (child, dest_image);
    }
#endif
  GIMP_ITEM_CLASS (Class::parent_class)->convert (GIMP_ITEM(g_object), dest_image);
}

void GLib::FilterLayer::translate (gint      offset_x,
                                     gint      offset_y,
                                     gboolean  push_undo)
{
  GimpLayerMask         *mask;
  GList                 *list;

  GIMP_ITEM_CLASS(Class::parent_class)->translate(GIMP_ITEM(g_object), offset_x, offset_y, push_undo);
  filter_reset();
}

void GLib::FilterLayer::scale (gint                   new_width,
                                 gint                   new_height,
                                 gint                   new_offset_x,
                                 gint                   new_offset_y,
                                 GimpInterpolationType  interpolation_type,
                                 GimpProgress          *progress)
{
  GimpItem*              item = GIMP_ITEM(g_object);
  GIMP_ITEM_CLASS(Class::parent_class)->scale(GIMP_ITEM(g_object), new_width, new_height, new_offset_x, new_offset_y, interpolation_type, progress);
  filter_reset();
}

void GLib::FilterLayer::resize (GimpContext *context,
                                  gint         new_width,
                                  gint         new_height,
                                  gint         offset_x,
                                  gint         offset_y)
{
  GimpItem*              item = GIMP_ITEM(g_object);
  GIMP_ITEM_CLASS(Class::parent_class)->resize(GIMP_ITEM(g_object), context, new_width, new_height, offset_x, offset_y);
  filter_reset();
}

void GLib::FilterLayer::flip (GimpContext         *context,
                                GimpOrientationType  flip_type,
                                gdouble              axis,
                                gboolean             clip_result)
{
  filter_reset();
}

void GLib::FilterLayer::rotate (GimpContext      *context,
                                  GimpRotationType  rotate_type,
                                  gdouble           center_x,
                                  gdouble           center_y,
                                  gboolean          clip_result)
{
  GimpItem*              item = GIMP_ITEM(g_object);
  GIMP_ITEM_CLASS(Class::parent_class)->rotate(GIMP_ITEM(g_object), context, rotate_type, center_x, center_y, clip_result);
  filter_reset();
}

void GLib::FilterLayer::transform (GimpContext            *context,
                                     const GimpMatrix3      *matrix,
                                     GimpTransformDirection  direction,
                                     GimpInterpolationType   interpolation_type,
                                     gint                    recursion_level,
                                     GimpTransformResize     clip_result,
                                     GimpProgress           *progress)
{
  GimpItem*              item = GIMP_ITEM(g_object);
  GIMP_ITEM_CLASS(Class::parent_class)->transform(GIMP_ITEM(g_object), context, matrix, direction, interpolation_type, recursion_level, clip_result, progress);
  filter_reset();
}

gint64 GLib::FilterLayer::estimate_memsize (gint                width,
                                              gint                height)
{
  GList                 *list;
  GimpImageBaseType      base_type;
  gint64                 memsize = 0;
  base_type = (GimpImageBaseType)(GIMP_IMAGE_TYPE_BASE_TYPE (gimp_drawable_type (GIMP_DRAWABLE(g_object))));

  memsize += gimp_projection_estimate_memsize (base_type, width, height);

  return memsize + GIMP_DRAWABLE_CLASS (Class::parent_class)->estimate_memsize (GIMP_DRAWABLE(g_object),
                                                                         width,
                                                                         height);
}

void GLib::FilterLayer::project_region (gint          x,
                                          gint          y,
                                          gint          width,
                                          gint          height,
                                          PixelRegion  *projPR,
                                          gboolean      combine)
{
  GimpDrawable  *drawable     = GIMP_DRAWABLE(g_object);
  GimpLayer     *layer        = GIMP_LAYER (drawable);
  GimpLayerMask *mask         = gimp_layer_get_mask (layer);
  const gchar*   name         = gimp_object_get_name(GIMP_OBJECT(layer));
  GimpItem*      item         = GIMP_ITEM(g_object);
  gint           x1           = x;
  gint           y1           = y;
  gint           offset_x     = gimp_item_get_offset_x(item);
  gint           offset_y     = gimp_item_get_offset_y(item);
  gint           w            = gimp_item_get_width(item);
  gint           h            = gimp_item_get_height(item);
  gint           parent_off_x = 0;
  gint           parent_off_y = 0;
  GimpViewable*  parent       = gimp_viewable_get_parent(GIMP_VIEWABLE(g_object));

  if (parent)
    gimp_item_get_offset(GIMP_ITEM(parent), &parent_off_x, &parent_off_y);

//  g_print("FilterLayer::project_region:%s: (x: %d, y: %d)+(w: %d, h: %d) / (W: %d, H: %d)\n", name, x, y, width, height, w, h);
//  g_print("                                parent-offset:%d,%d,  item-offset:%d,%d\n", parent_off_x, parent_off_y, offset_x, offset_y);
//  g_print("           In projection coord: (x: %d, y: %d)\n", x + offset_x - parent_off_x, y + offset_y - parent_off_y);

  // Region is copied to projected tiles
  for (GList* list = updates; list; ) {
    GList* next = g_list_next(list);
    auto rect = (Rectangle*)list->data;
    if (x1 + offset_x - parent_off_x <= rect->x &&
        y1 + offset_y - parent_off_y <= rect->y &&
        rect->x + rect->width  <= x1 + offset_x - parent_off_x + width &&
        rect->y + rect->height <= y1 + offset_y - parent_off_y + height) {
//      g_print("  compared rect: %d,%d +(%d,%d)\n", rect->x, rect->y, rect->width, rect->height);
//      g_print("  In drawable coord: (x: %d, y: %d)\n", rect->x - offset_x + parent_off_x, rect->y - offset_y + parent_off_y);
      projected_tiles_updated = true;

      if (!projected_tiles) {
        /*  Allocate the new tiles  */
        GimpItem*      item     = GIMP_ITEM(g_object);
        GimpImageType  new_type = gimp_drawable_type_with_alpha (GIMP_DRAWABLE(g_object));
        projected_tiles         = tile_manager_new (w, h,
                                                    GIMP_IMAGE_TYPE_BYTES (new_type));
      }
      PixelRegion tilePR;
      PixelRegion read_proj_PR;
      pixel_region_init (&tilePR,       projected_tiles,
                         rect->x + parent_off_x - offset_x, rect->y + parent_off_y - offset_y,
                         rect->width, rect->height, TRUE);
      pixel_region_init (&read_proj_PR, projPR->tiles,
                         rect->x,                           rect->y,
                         rect->width, rect->height, FALSE);
      copy_region_nocow(&read_proj_PR, &tilePR);

      updates = g_list_delete_link (updates, list);
      delete rect;
    }
    list = next;
  }

  // Filter effects is applied to updated layer image
  if (projected_tiles_updated) {
    if (!runner || !runner->is_running()) {
      TileManager* tiles   = gimp_drawable_get_tiles(drawable);
      PixelRegion  srcPR, destPR;

      pixel_region_init (&destPR, tiles          , 0, 0, w, h, TRUE);
      pixel_region_init (&srcPR , projected_tiles, 0, 0, w, h, FALSE);
      copy_region_nocow(&srcPR, &destPR);

      if (runner) {
        g_print("start runner\n");
        runner->run(GIMP_ITEM(g_object));
      }
      projected_tiles_updated = false;

    } else {
//      g_print("FilterLayer::project_region: skip filter execution: filter_active=%p, updates=%p\n", filter_active, updates);
//      for (GList* list = updates; list; list = g_list_next(list)) {
//        auto r = (Rectangle*)list->data;
//        g_print("  skipped: %d, %d + (%d, %d)\n", r->x, r->y, r->width, r->height);
//      }
    }

  } else {
//    g_print ("FilterLayer::project_region:  not updated %d, %d +(%d, %d)\n", x, y, width, height);
  }

  if (runner && runner->is_running() || projected_tiles_updated)
    return;

  GIMP_DRAWABLE_CLASS(Class::parent_class)->project_region (drawable, x, y, width, height, projPR, combine);
}

void GLib::FilterLayer::set_procedure(const char* proc_name, GValueArray* args)
{
  if (runner) {
    delete runner;
    runner = NULL;
  }
  auto           image   = _G( gimp_item_get_image(GIMP_ITEM(g_object)) );
  Gimp*          gimp    = image->gimp;
  GimpPDB*       pdb     = gimp->pdb;
  GimpProcedure* proc    = gimp_pdb_lookup_procedure(pdb, proc_name);
  ProcedureRunner* r     = new ProcedureRunner(proc, GIMP_PROGRESS(g_object));

  if (args) {
    for (int i = 0; i < proc->num_args; i ++)
      r->set_arg(i, args->values[i]);
  }

  runner     = r;

  invalidate_layer();
}

const char* GLib::FilterLayer::get_procedure()
{
  if (runner)
    return runner->get_procedure_name();
  return "";
}

GValueArray* GLib::FilterLayer::get_procedure_args()
{
  if (runner)
    runner->get_args();
  return  NULL;
}

GValue GLib::FilterLayer::get_procedure_arg(int index)
{
  if (runner)
    runner->get_arg(index);
  return  G_VALUE_INIT;
}

void GLib::FilterLayer::set_procedure_arg(int index, GValue value)
{
  if (runner)
    runner->set_arg(index, value);
}


gint GLib::FilterLayer::get_opacity_at (gint          x,
                                          gint          y)
{
  /* Only consider child layers as having content */
  return 0;
}

void GLib::FilterLayer::suspend_resize (gboolean        push_undo) {
  GimpItem *item;

  item = GIMP_ITEM (g_object);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    gimp_image_undo_push_filter_layer_suspend (gimp_item_get_image (item),
                                              NULL, Class::Traits::cast(g_object));
  
  suspend_resize_++;
}

void GLib::FilterLayer::resume_resize (gboolean        push_undo) {
  GimpItem              *item;
  g_return_if_fail (suspend_resize_ > 0);

  item = GIMP_ITEM (g_object);

  if (! gimp_item_is_attached (item))
    push_undo = FALSE;
  if (push_undo)
    gimp_image_undo_push_filter_layer_resume (gimp_item_get_image (item),
                                             NULL, Class::Traits::cast(g_object));
  suspend_resize_--;

  if (suspend_resize_ == 0)
    {
      update_size ();
    }
}


/*  priv functions  */
void GLib::FilterLayer::on_parent_changed (GimpViewable  *viewable,
                                             GimpViewable  *parent)
{
//  g_print("on_parent_changed: %p ==> parent=%s\n", viewable, parent? gimp_object_get_name(parent): NULL);
  if (child_update_conn) {
    delete child_update_conn;
    child_update_conn = NULL;
  }
  if (parent) {
    GimpContainer* children = gimp_viewable_get_children(parent);
    child_update_conn =
      g_signal_connect_delegator (G_OBJECT(children), "update",
                                  Delegators::delegator(this, &GLib::FilterLayer::on_stack_update));
  } else {
    GimpImage*   image   = gimp_item_get_image(GIMP_ITEM(g_object));
    GimpContainer* container = gimp_image_get_layers(image);
    child_update_conn =
      g_signal_connect_delegator (G_OBJECT(container), "update",
                                  Delegators::delegator(this, &GLib::FilterLayer::on_stack_update));
  }
}

static void delete_rectangle(void* rect){
  delete (GLib::FilterLayer::Rectangle*)rect;
}
void GLib::FilterLayer::filter_reset () {
  g_print("FilterLayer::filter_reset\n");
  gimp_progress_cancel(GIMP_PROGRESS(g_object));
  g_list_free_full(updates, delete_rectangle);
  updates = NULL;
  tile_manager_unref(projected_tiles);
  projected_tiles = NULL;
}


void GLib::FilterLayer::update () {
  if (suspend_resize_ == 0){
    update_size ();
  }
}

void GLib::FilterLayer::update_size () {
  GimpItem              *item       = GIMP_ITEM (g_object);
  gint                   old_x      = gimp_item_get_offset_x (item);
  gint                   old_y      = gimp_item_get_offset_y (item);
  gint                   old_width  = gimp_item_get_width  (item);
  gint                   old_height = gimp_item_get_height (item);
  gint                   x          = 0;
  gint                   y          = 0;
  gint                   width      = 1;
  gint                   height     = 1;
  gboolean               first      = TRUE;
  GList                 *list;

  g_print ("%s (%s) %d, %d (%d, %d)\n",
           G_STRFUNC, gimp_object_get_name (g_object),
           x, y, width, height);

  if (reallocate_projection ||
      x      != old_x                ||
      y      != old_y                ||
      width  != old_width            ||
      height != old_height)
    {
      if (reallocate_projection ||
          width  != old_width            ||
          height != old_height)
        {
          TileManager *tiles;

          reallocate_projection = FALSE;

          /*  temporarily change the return values of gimp_viewable_get_size()
           *  so the projection allocates itself correctly
           */
          reallocate_width  = width;
          reallocate_height = height;
          invalidate_layer();
          gimp_projectable_structure_changed (GIMP_PROJECTABLE (g_object));

        }
      else
        {
          gimp_item_set_offset (item, x, y);

          /*  invalidate the entire projection since the position of
           *  the children relative to each other might have changed
           *  in a way that happens to leave the group's width and
           *  height the same
           */
          invalidate_layer();
          gimp_projectable_invalidate (GIMP_PROJECTABLE (g_object),
                                       x, y, width, height);

          /*  see comment in gimp_filter_layer_stack_update() below  */
//          gimp_pickable_flush (GIMP_PICKABLE (projection));
        }
    }
}

GimpProgress* GLib::FilterLayer::start(const gchar* message,
                                         gboolean     cancelable)
{
  g_print("FilterLayer::start: %s\n", message);
  return GIMP_PROGRESS(g_object);
}

void GLib::FilterLayer::end()
{
  g_print("FilterLayer::end\n");
  runner->on_end();
  if (new_runner) {
    delete runner;
    runner      = new_runner;
    new_runner = NULL;
    filter_reset();
    return;
  }
//  filter_active = NULL;
  GimpItem* item   = GIMP_ITEM(g_object);
  gint      width  = gimp_item_get_width(item);
  gint      height = gimp_item_get_height(item);
  gimp_drawable_update (GIMP_DRAWABLE (g_object),
                        0, 0, width, height);
  GimpImage*   image   = gimp_item_get_image(GIMP_ITEM(g_object));
  gimp_image_flush (image);
}

gboolean GLib::FilterLayer::is_active()
{
  return runner && runner->is_running();
//  return filter_active != NULL;
}

void GLib::FilterLayer::set_text(const gchar* message)
{
  g_print("FilterLayer::set_text: %s\n", message);
}

void GLib::FilterLayer::set_value(gdouble      percentage)
{
  filter_progress = percentage;
}

gdouble GLib::FilterLayer::get_value()
{
  return filter_progress;
}

void GLib::FilterLayer::pulse()
{
  g_print("FilterLayer::pulse\n");
}

gboolean GLib::FilterLayer::message(Gimp*                gimp,
                                      GimpMessageSeverity severity,
                                      const gchar*        domain,
                                      const gchar*       message)
{
  g_print("FilterLayer::message: %s / %s\n", domain, message);
  return TRUE;
}

void GLib::FilterLayer::invalidate_area (gint               x,
                                         gint               y,
                                         gint               width,
                                         gint               height)
{
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

  for (int x3 = x1; x3 < x2; x3 += TILE_WIDTH - (x3 % TILE_WIDTH)) {
    for (int y3 = y1; y3 < y2; y3 += TILE_HEIGHT - (y3 % TILE_HEIGHT)) {
      gint w = MIN(TILE_WIDTH  - (x3 % TILE_WIDTH),  x2 - x3);
      gint h = MIN(TILE_HEIGHT - (y3 % TILE_HEIGHT), y2 - y3);

      bool found = false;
      for (GList* list = updates; list; list = g_list_next(list)) {
        auto rect = (Rectangle*)list->data;
        if (rect->x == x3 && rect->y == y3 &&
            rect->width == w && rect->height == h) {
          found = true;
          break;
        }
      }

      if (!found) {
//        g_print ("FilterLayer::on_stack_update ::::: added ==> %d, %d +(%d, %d)\n",
//                 x3, y3, w, h);
//        g_print ("                                   parent-offset:%d,%d, item-offset:%d,%d\n", parent_off_x, parent_off_y, offset_x, offset_y);
        updates = g_list_append(updates, new Rectangle(x3, y3, w, h));
      }
    }
  }
}

void GLib::FilterLayer::invalidate_layer ()
{
  auto self         = _G(g_object);
  auto image        = _G( gimp_item_get_image(GIMP_ITEM(g_object)) );
  gint width        = self [gimp_item_get_width] ();
  gint height       = self [gimp_item_get_height] ();
  gint offset_x     = self [gimp_item_get_offset_x] ();
  gint offset_y     = self [gimp_item_get_offset_y] ();

  invalidate_area(offset_x, offset_y, width, height);

  image [gimp_image_invalidate] (offset_x, offset_y, width, height, 0);
  image [gimp_image_flush] ();
}

void GLib::FilterLayer::on_stack_update (GimpDrawableStack *stack,
                                           gint               x,
                                           gint               y,
                                           gint               width,
                                           gint               height,
                                           GimpItem          *item)
{
  if (projected_tiles && G_OBJECT(item) == g_object)
    return;
  gint item_index = gimp_container_get_child_index(GIMP_CONTAINER(stack), GIMP_OBJECT(item));
  gint self_index = gimp_container_get_child_index(GIMP_CONTAINER(stack), GIMP_OBJECT(g_object));

  if (item_index < self_index)
    return;

  invalidate_area(x, y, width, height);
}

void GLib::FilterLayer::on_proj_update (GimpProjection *proj,
                                          gboolean        now,
                                          gint            x,
                                          gint            y,
                                          gint            width,
                                          gint            height)
{
  g_print ("%s (%s) %d, %d (%d, %d)\n",
           G_STRFUNC, gimp_object_get_name (g_object),
           x, y, width, height);

  /*  the projection speaks in image coordinates, transform to layer
   *  coordinates when emitting our own update signal.
   */
  gimp_drawable_update (GIMP_DRAWABLE (g_object),
                        x - gimp_item_get_offset_x (GIMP_ITEM (g_object)),
                        y - gimp_item_get_offset_y (GIMP_ITEM (g_object)),
                        width, height);
}

/*  public functions  */

GimpLayer *
FilterLayerInterface::new_instance (GimpImage            *image,
                                    gint                  width,
                                    gint                  height,
                                    const gchar          *name,
                                    gdouble               opacity,
                                    GimpLayerModeEffects  mode)
{
  typedef GLib::Class F;
  GimpImageType   type;
  GimpFilterLayer *group;

  g_return_val_if_fail (GIMP_IS_IMAGE (image), NULL);

  type = gimp_image_base_type_with_alpha (image);

  g_print("create instance type=%lx\n", GLib::Class::Traits::get_type());
  group = F::Traits::cast (gimp_drawable_new (GLib::Class::Traits::get_type(),
                                               image, name,
                                               0, 0, width, height,
                                               type));
  GimpLayer* layer = GIMP_LAYER(group);
  opacity = CLAMP (opacity, GIMP_OPACITY_TRANSPARENT, GIMP_OPACITY_OPAQUE);

  layer->opacity = opacity;
  layer->mode    = mode;

  g_print("new_instance::group=%p\n", group);

  return GIMP_LAYER (group);
}

FilterLayerInterface* FilterLayerInterface::cast(gpointer obj)
{
  if (!is_instance(obj))
    return NULL;
  return dynamic_cast<FilterLayerInterface*>(GLib::Class::get_private(obj));
}

bool FilterLayerInterface::is_instance(gpointer obj)
{
  return GLib::Class::Traits::is_instance(obj);
}

GType gimp_filter_layer_get_type() {
  return GLib::Class::Traits::get_type();
}

GimpLayer      * gimp_filter_layer_new            (GimpImage            *image,
                                                   gint                  width,
                                                   gint                  height,
                                                   const gchar          *name,
                                                   gdouble               opacity,
                                                   GimpLayerModeEffects  mode)
{
  return FilterLayerInterface::new_instance(image, width, height, name, opacity, mode);
}

void             gimp_filter_layer_suspend_resize (GimpFilterLayer *group,
                                                  gboolean        push_undo)
{
  FilterLayerInterface::cast(group)->suspend_resize(push_undo);
}
void             gimp_filter_layer_resume_resize  (GimpFilterLayer *group,
                                                  gboolean        push_undo)
{
  FilterLayerInterface::cast(group)->resume_resize(push_undo);
}
