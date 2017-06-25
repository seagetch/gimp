/* GIMP-painter
 * GimpCellRendererPopup
 * Copyright (C) 2016  seagetch
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
#include "base/scopeguard.hpp"
#include "base/glib-cxx-utils.hpp"
#include "base/glib-cxx-impl.hpp"
#include <functional>

extern "C" {
#include <config.h>
#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets-types.h"
#include "core/gimpdashpattern.h"
#include "core/gimpmarshal.h"
#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"

#include "pdb/gimppdb-query.h"
#include "pdb/gimppdb.h"
#include "pdb/gimpprocedure.h"
#include "plug-in/gimppluginprocedure.h"

#include "libgimpwidgets/gimpwidgets.h"
#include "libgimpbase/gimpbase.h"

#include "gimp-intl.h"
#include "gimpwidgets-constructors.h"
#include "gimphelp-ids.h"
#include "gimpspinscale.h"
};

#include "base/glib-cxx-def-utils.hpp"

#include "gimpcellrendererpopup.h"
#include "popupper.h"

#include "core/gimpfilterlayer.h"

//////////////////////////////////////////////////////////////////////////////////////////////
class PopupWindow {
  enum {
    LABEL,
    VALUE,
    NUM_ARGS
  };
  GWrapper<GimpLayer> layer;
  GWrapper<GtkWidget> mode_select;
  GWrapper<GtkWidget> filter_select;
  GWrapper<GtkWidget> filter_edit;

  GtkWidget* create_label_tree_view(const gchar* title, GtkTreeModel* model) {
    GtkWidget* result    = gtk_tree_view_new_with_model(model);
    auto       tree_view = _G(result);
    auto       column    = _G(gtk_tree_view_column_new_with_attributes (title,
        gtk_cell_renderer_text_new(),
        "text", LABEL,
        NULL));
    tree_view[gtk_tree_view_append_column](column.ptr());
    return tree_view.ptr();
  };

  void add_proc(GWrapper<GtkTreeStore> store, GtkTreeIter* iter, GtkTreeIter* parent, GimpPlugInProcedure* proc, const gchar* label = NULL) {
    const gchar* desc = proc? proc->menu_label : label;
    g_print("PopupWindow::add_proc: desc=%s, parent=%p, proc=%s\n", desc, parent, proc? GIMP_PROCEDURE(proc)->original_name: "(null)");
    store [gtk_tree_store_append] (iter, parent);
    gtk_tree_store_set (GTK_TREE_STORE(store.ptr()), iter, VALUE, proc? GIMP_PROCEDURE(proc)->original_name: "", LABEL, _(desc), -1);
  };

  bool is_allowed_proc_path(gchar* menu_path) {
    static const gchar* white_list_prefix[] = {
        "<Image>/Filters/",
        "<Image>/Colors/",
        NULL
    };
    static const gchar* black_list_prefix[] = {
        "<Image>/Filters/Animation",
        "<Image>/Filters/Render",
        "<Image>/Filters/Language",
        "<Image>/Filters/Web",
        NULL
    };
    for (const gchar** i = white_list_prefix; *i; i ++) {
      if (strncmp(menu_path, *i, strlen(*i)) == 0) {
        bool result = true;
        for (const gchar** j = black_list_prefix; *j; j ++) {
          if (strncmp(menu_path, *j, strlen(*j)) == 0) {
            result = false;
            break;
          }
        }
        if (result)
          return true;
      }
    }
    return false;
  }

  GtkWidget* create_filter_list(GWrapper<GimpLayer> layer) {
    auto             store    = _G(gtk_tree_store_new(NUM_ARGS, G_TYPE_STRING, G_TYPE_STRING));
    auto             pdb      = _G( layer [gimp_item_get_image] ().ptr()->gimp->pdb );
    gint             num_procs;
    gchar**          procs;
    gboolean query = pdb [gimp_pdb_query] (".*", ".*", ".*", ".*", ".*", ".*", ".*", &num_procs, &procs, NULL);

    GHashTableHolder<gchar*, GtkTreeIter*> tree_map = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

    for (int i = 0; i < num_procs; i ++) {
      gchar*         proc_name        = procs[i];
      GimpProcedure* proc             = pdb [gimp_pdb_lookup_procedure] (proc_name);

      g_free(proc_name);

      if (!proc || !GIMP_IS_PLUG_IN_PROCEDURE(proc)) continue;

      GimpPlugInProcedure* plugin_proc = GIMP_PLUG_IN_PROCEDURE(proc);
      for (GList* l = plugin_proc->menu_paths; l; l = g_list_next(l)) {
        gchar* path = (gchar*)l->data;

        if (is_allowed_proc_path(path)) {

          // Registers procedure for filter selection list.
          StringListHolder path_list    = g_strsplit(path, "/", -1);
          GStringHolder    current_path = g_string_new("");
          GtkTreeIter*     parent       = NULL;

          // Traverses appropriate GtkTreeIter folder for menu_path
          for (int p = 0; path_list[p]; p ++) {
            if (p == 0) continue;
            g_string_append_c(current_path.ptr(), '/');
            g_string_append(current_path.ptr(), path_list[p]);
            if (tree_map[current_path.str()])
              parent = tree_map[current_path.str()];
            else {
              GtkTreeIter* cur_iter = g_new0(GtkTreeIter, 1);
              add_proc(store, cur_iter, parent, NULL, path_list[p]);
              tree_map.insert(g_strdup(current_path.str()), cur_iter);
              parent = cur_iter;
            }
          }

          // Registers filter
          GtkTreeIter proc_iter;
          add_proc(store, &proc_iter, parent, plugin_proc);
          break;
        }
      }

    }
    g_free(procs);

    filter_select = create_label_tree_view(_("Procedures"), GTK_TREE_MODEL(store.ptr()));
    return filter_select;
  }

  void on_filter_list_changed(GtkWidget* widget) {
    GtkTreeModel* model     = NULL;
    GtkTreeIter   iter;
    gchar*        proc_name = NULL;

    auto selection = _G(widget);
    selection [gtk_tree_selection_get_selected] (&model, &iter);
    gtk_tree_model_get (model, &iter, VALUE, &proc_name, -1);
    if (proc_name && strlen(proc_name) > 0) {
      g_print("CHANGED: proc_name=%s\n", proc_name);
      update_filter_edit(proc_name);
    }
  }

  void update_filter_edit(gchar* proc_name) {
    auto           pdb  = _G( layer [gimp_item_get_image] ().ptr()->gimp->pdb );
    GimpProcedure* proc = pdb [gimp_pdb_lookup_procedure] (proc_name);

    if (!GIMP_IS_PLUG_IN_PROCEDURE(proc)) return;

    GListHolder children( filter_edit [gtk_container_get_children] () );
    for(GList* iter = children.ptr(); iter != NULL; iter = g_list_next(iter))
      gtk_widget_destroy(GTK_WIDGET(iter->data));

    for (int j = 0; j < proc->num_args; j ++) {
      GParamSpec*  pspec         = proc->args[j];
      const gchar* arg_blurb     = g_param_spec_get_blurb(pspec);
      GType        arg_type      = G_PARAM_SPEC_TYPE(pspec);
      const gchar* arg_type_name = G_PARAM_SPEC_TYPE_NAME(pspec);

      if (GIMP_IS_PARAM_SPEC_IMAGE_ID(pspec) ||
          GIMP_IS_PARAM_SPEC_DRAWABLE_ID(pspec) ||
          GIMP_IS_PARAM_SPEC_ITEM_ID(pspec) ||
          strcmp(g_param_spec_get_name(pspec), "run-mode") == 0)
        continue;

      with (filter_edit, [&](auto it) {
        it.pack_start ( gtk_label_new(_(arg_blurb)), FALSE, FALSE, 0, [&](auto arg_name_label) {
          arg_name_label [gtk_widget_show] ();
        });
        it.pack_start ( gtk_label_new(_(arg_type_name)), FALSE, FALSE, 0, [&](auto arg_type_label) {
          arg_type_label [gtk_widget_show] ();
        });
      });
    }
  }

  GtkWidget* create_filter_edit(GWrapper<GimpLayer> layer) {
    filter_edit = with (gtk_box_new(GTK_ORIENTATION_VERTICAL, 0), [&](auto vbox) {
      vbox [gtk_widget_show] ();
    });
    return filter_edit;
  }

  GtkWidget* create_paint_mode_list() {
    static GEnumClass* klass = G_ENUM_CLASS(g_type_class_ref(GIMP_TYPE_LAYER_MODE_EFFECTS));
    auto               store = _G(gtk_tree_store_new(NUM_ARGS, G_TYPE_STRING, GIMP_TYPE_LAYER_MODE_EFFECTS));

    auto group = [&](GtkTreeIter* parent, const gchar* label, auto f) {
      GtkTreeIter iter;
      g_print("PopupWindow::add_mode: desc=%s, value=%d\n", label, -1);
      store [gtk_tree_store_append] (&iter, parent);
      gtk_tree_store_set (GTK_TREE_STORE(store.ptr()), &iter, VALUE, -1, LABEL, label, -1);
      f(&iter);
    };

    auto mode  = [&](GtkTreeIter* parent, GimpLayerModeEffects value) {
      GtkTreeIter  iter;
      GEnumValue*  v    = g_enum_get_value (klass, (gint) value);
      const gchar* desc = _(gimp_enum_value_get_desc (klass, v));
      g_print("PopupWindow::add_mode: desc=%s, value=%d\n", desc, (gint)value);
      store [gtk_tree_store_append] (&iter, parent);
      gtk_tree_store_set (GTK_TREE_STORE(store.ptr()), &iter, VALUE, value, LABEL, desc, -1);
    };

    group(NULL, _("Normal"), [&](auto parent) {
      mode( parent, GIMP_NORMAL_MODE);
      mode( parent, GIMP_DISSOLVE_MODE);
    });
    group(NULL, _("Lighten"), [&](auto parent) {
      mode( parent, GIMP_LIGHTEN_ONLY_MODE);
      mode( parent, GIMP_SCREEN_MODE);
      mode( parent, GIMP_DODGE_MODE);
      mode( parent, GIMP_ADDITION_MODE);
    });
    group(NULL, _("Darken"), [&](auto parent) {
      mode( parent, GIMP_DARKEN_ONLY_MODE);
      mode( parent, GIMP_MULTIPLY_MODE);
      mode( parent, GIMP_BURN_MODE);
    });
    group(NULL, _("Emphasis"), [&](auto parent) {
      mode( parent, GIMP_OVERLAY_MODE);
      mode( parent, GIMP_SOFTLIGHT_MODE);
      mode( parent, GIMP_HARDLIGHT_MODE);
    });
    group(NULL, _("Inversion"), [&](auto parent) {
      mode( parent, GIMP_DIFFERENCE_MODE);
      mode( parent, GIMP_SUBTRACT_MODE);
      mode( parent, GIMP_GRAIN_EXTRACT_MODE);
      mode( parent, GIMP_GRAIN_MERGE_MODE);
      mode( parent, GIMP_DIVIDE_MODE);
    });
    group(NULL, _("Color"), [&](auto parent) {
      mode( parent, GIMP_HUE_MODE);
      mode( parent, GIMP_SATURATION_MODE);
      mode( parent, GIMP_COLOR_MODE);
      mode( parent, GIMP_VALUE_MODE);
    });
    group(NULL, _("Masking"), [&](auto parent) {
      mode( parent, GIMP_SRC_IN_MODE);
      mode( parent, GIMP_DST_IN_MODE);
      mode( parent, GIMP_SRC_OUT_MODE);
      mode( parent, GIMP_DST_OUT_MODE);
    });

    mode_select = create_label_tree_view(_("Mode"), GTK_TREE_MODEL(store.ptr()));
    return mode_select;
  };

public:
  void create_view(GtkWidget* widget, GtkWidget** result, gpointer data) {
    PangoAttribute        *attr;

    layer = GIMP_LAYER(data);
    *result = with (gtk_box_new(GTK_ORIENTATION_VERTICAL, 0), [&](auto vbox) {
      vbox.pack_start (gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), TRUE, FALSE, 0, [&](auto hbox) {
        hbox [gtk_widget_show] ();

        /*  Opacity scale  */

        g_print("opacity adjustment\n");
        hbox.pack_start(gimp_spin_scale_new (
            with (gtk_adjustment_new (100.0, 0.0, 100.0, 1.0, 10.0, 0.0),[&](auto it) {
//              it.connect("value-changed", Delegator::delegator(this, &PopupWindow::));
            }), _("Opacity"), 1), TRUE, TRUE, 0, [&](auto it) {
          it [gimp_help_set_help_data] (NULL, GIMP_HELP_LAYER_DIALOG_OPACITY_SCALE);
        });
        /*  Lock alpha toggle  */
        hbox.pack_start (gtk_toggle_button_new(), FALSE, FALSE, 0, [&] (auto it) {
          it [gtk_widget_show] ();
//          it.connect("toggled", Delegator::delegator(this, &PopupWindow::));
          it [gimp_help_set_help_data] ( _("Lock alpha channel"), GIMP_HELP_LAYER_DIALOG_LOCK_ALPHA_BUTTON);

          auto icon_size = GTK_ICON_SIZE_BUTTON;
          it.add (gtk_image_new_from_stock (GIMP_STOCK_TRANSPARENCY, icon_size), [&](auto image) {
            image [gtk_widget_show] ();
          });
        });
        /*
            view->priv->italic_attrs = pango_attr_list_new ();
            attr = pango_attr_style_new (PANGO_STYLE_ITALIC);
            attr->start_index = 0;
            attr->end_index   = -1;
            pango_attr_list_insert (view->priv->italic_attrs, attr);

            view->priv->bold_attrs = pango_attr_list_new ();
            attr = pango_attr_weight_new (PANGO_WEIGHT_BOLD);
            attr->start_index = 0;
            attr->end_index   = -1;
            pango_attr_list_insert (view->priv->bold_attrs, attr);
        */
      });
      vbox.pack_start (gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0), TRUE, TRUE, 0, [&](auto hbox) {
        hbox [gtk_widget_show] ();

        /*  Paint mode menu  */
        g_print("create paint_mode_menu\n");
        hbox.pack_start (gtk_scrolled_window_new(NULL, NULL), TRUE, TRUE, 0, [&](auto it) {
          it [gtk_widget_show]();
          it.add (this->create_paint_mode_list(), [&](auto it) {
            it [gimp_help_set_help_data] (NULL, GIMP_HELP_LAYER_DIALOG_PAINT_MODE_MENU);
            it [gtk_tree_view_expand_all] ();
            it [gtk_widget_show]();
//            it.connect("changed", Delegator::delegator(this, &PopupWindow::));
          });
          GtkRequisition req;
          vbox[gtk_widget_get_requisition](&req);

          req.width  = MAX(req.width, 150);
          req.height = MAX(req.height, 200);
          it [gtk_widget_set_size_request] (req.width, req.height);
        });

        if (FilterLayerInterface::is_instance(layer)) {
          auto filter_layer = FilterLayerInterface::cast(layer);

          hbox.pack_start (gtk_scrolled_window_new(NULL, NULL), TRUE, TRUE, 0, [&](auto it) {
            it [gtk_widget_show] ();
            GtkRequisition req;
            vbox [gtk_widget_get_requisition] (&req);
            req.width  = MAX(req.width, 200);
            req.height = MAX(req.height, 300);
            it [gtk_widget_set_size_request] (req.width, req.height);

            it.add (this->create_filter_list(layer), [&](auto it) {
              it [gtk_widget_show] ();
              it [gtk_tree_view_expand_all] ();
              auto selection = it [gtk_tree_view_get_selection] ();
              selection.connect("changed", Delegator::delegator(this, &PopupWindow::on_filter_list_changed));
            });
          });

          hbox.pack_start (gtk_scrolled_window_new(NULL, NULL), TRUE, TRUE, 0, [&](auto it) {
            it [gtk_widget_show] ();
            GtkRequisition req;
            vbox [gtk_widget_get_requisition] (&req);
            req.width  = MAX(req.width, 200);
            req.height = MAX(req.height, 300);
            it[gtk_widget_set_size_request](req.width, req.height);

            it.add (this->create_filter_edit(layer), [&](auto it) {
              it [gtk_widget_show] ();
            });
          });
        }
      });

      vbox [gtk_widget_show_all] ();
    });
  };
};
//////////////////////////////////////////////////////////////////////////////////////////////
namespace GtkCXX {
typedef Traits<GtkCellRenderer, GtkCellRendererClass, gtk_cell_renderer_get_type> ParentTraits;
typedef ClassHolder<ParentTraits, GimpCellRendererPopup, GimpCellRendererPopupClass> ClassDef;

struct CellRendererPopup : virtual public ImplBase, virtual public CellRendererPopupInterface
{
  enum
  {
    CLICKED,
    LAST_SIGNAL
  };
  enum
  {
    PROP_0,
    PROP_ACTIVE
  };
  static guint signals[LAST_SIGNAL];
  GdkPixbuf*  pixbuf;
  gchar*      stock_id;
  GtkIconSize stock_size;
  bool        active;


  CellRendererPopup(GObject* o) : ImplBase(o) {}
  virtual ~CellRendererPopup() {};

  static void class_init(ClassDef::Class* klass);

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
  virtual void            get_size     (GtkWidget       *widget,
                                        GdkRectangle    *cell_area,
                                        gint            *x_offset,
                                        gint            *y_offset,
                                        gint            *width,
                                        gint            *height);
  void                    create_pixbuf (GtkWidget           *widget);
  virtual void            render       (GdkWindow            *window,
                                        GtkWidget            *widget,
                                        GdkRectangle         *background_area,
                                        GdkRectangle         *cell_area,
                                        GdkRectangle         *expose_area,
                                        GtkCellRendererState  flags);
  virtual gboolean        activate     (GdkEvent             *event,
                                        GtkWidget            *widget,
                                        const gchar          *path,
                                        GdkRectangle         *background_area,
                                        GdkRectangle         *cell_area,
                                        GtkCellRendererState  flags);
  virtual void            clicked      (GObject* widget, const gchar* path, GdkRectangle* cell_area);
};
guint CellRendererPopup::signals[CellRendererPopup::LAST_SIGNAL] = {0};

extern const char gimp_cell_renderer_popup_name[] = "GimpCellRendererPopup";
typedef ClassDefinition<gimp_cell_renderer_popup_name, ClassDef, CellRendererPopup> Class;

#define bind_to_class(klass, method, impl)  Class::__(&klass->method).bind<&impl::method>()

void CellRendererPopup::class_init(ClassDef::Class *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  bind_to_class (object_class, finalize    , CellRendererPopup);
  bind_to_class (object_class, constructed , CellRendererPopup);
  bind_to_class (object_class, get_property, CellRendererPopup);
  bind_to_class (object_class, set_property, CellRendererPopup);
  bind_to_class (cell_class  , activate    , CellRendererPopup);
  bind_to_class (cell_class  , get_size    , CellRendererPopup);
  bind_to_class (cell_class  , render      , CellRendererPopup);

  signals[CLICKED] =
    g_signal_new ("parent-clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (GimpCellRendererPopupClass, clicked),
                  NULL, NULL,
                  gimp_marshal_VOID__OBJECT_STRING_POINTER,
                  G_TYPE_NONE, 3, G_TYPE_OBJECT, G_TYPE_STRING, G_TYPE_POINTER );

  g_object_class_install_property (object_class,
                                   PROP_ACTIVE,
                                   g_param_spec_boolean ("active",
                                                     NULL, NULL,
                                                     FALSE,
                                                     (GParamFlags)(GIMP_PARAM_READWRITE | G_PARAM_CONSTRUCT)));


  ImplBase::class_init<ClassDef::Class, CellRendererPopup>(klass);
}

}; // namespace

void GtkCXX::CellRendererPopup::init ()
{
  stock_id   = g_strdup(GIMP_STOCK_LAYER_MENU);
  stock_size = GTK_ICON_SIZE_MENU;
  g_print("CellRendererPopup::init\n");
}

void GtkCXX::CellRendererPopup::constructed ()
{
  PopupWindow* window_decor = new PopupWindow;
  decorator(GTK_WIDGET(g_object), window_decor);
  decorate_popupper(GTK_WIDGET(g_object), Delegator::delegator(window_decor, &PopupWindow::create_view));
}

void GtkCXX::CellRendererPopup::finalize ()
{
  if (stock_id) {
    g_free (stock_id);
    stock_id = NULL;
  }

  if (pixbuf) {
    g_object_unref (pixbuf);
    pixbuf = NULL;
  }

  G_OBJECT_CLASS (Class::parent_class)->finalize (g_object);
}

void GtkCXX::CellRendererPopup::set_property (guint         property_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  switch (property_id) {
  case PROP_ACTIVE:
    active = g_value_get_boolean (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
  }
}

void GtkCXX::CellRendererPopup::get_property (guint       property_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  switch (property_id) {
  case PROP_ACTIVE:
    g_value_set_boolean (value, active);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, property_id, pspec);
  }
}

void
GtkCXX::CellRendererPopup::get_size (GtkWidget       *widget,
                                     GdkRectangle    *cell_area,
                                     gint            *x_offset,
                                     gint            *y_offset,
                                     gint            *width,
                                     gint            *height)
{
  gfloat xalign, yalign;
  gint   xpad, ypad;
  GtkStyle *style  = gtk_widget_get_style (widget);
  auto self = _G(g_object);
  auto _x_offset = nullable(x_offset);
  auto _y_offset = nullable(y_offset);
  gint pixbuf_width, pixbuf_height;
  gint internal_width, internal_height;

  if (!pixbuf)
    create_pixbuf (widget);

  pixbuf_width  = gdk_pixbuf_get_width  (pixbuf);
  pixbuf_height = gdk_pixbuf_get_height (pixbuf);

  self[gtk_cell_renderer_get_alignment](&xalign, &yalign);
  self[gtk_cell_renderer_get_padding](&xpad, &ypad);

  internal_width  = (pixbuf_width  + (gint) xpad * 2 + style->xthickness * 2);
  internal_height = (pixbuf_height + (gint) ypad * 2 + style->ythickness * 2);

  if (cell_area) {
    gdouble align = ((_G(widget)[gtk_widget_get_direction]() == GTK_TEXT_DIR_RTL) ?
                    1.0 - xalign : xalign);

    _x_offset = MAX(align  * (cell_area->width  - internal_width),  0) + xpad;
    _y_offset = MAX(yalign * (cell_area->height - internal_height), 0) + ypad;
  } else {
    _x_offset = 0;
    _y_offset = 0;
  }

  *width  = internal_width  + 2 * xpad;
  *height = internal_height + 2 * ypad;

  g_print("CellRendererPopup::get_size: %d,%d\n", *width, *height);
}

void
GtkCXX::CellRendererPopup::create_pixbuf (GtkWidget              *widget)
{
  g_print("CellRendererPopup::create_pixbuf\n");
  if (pixbuf)
    g_object_unref (pixbuf);

  pixbuf = gtk_widget_render_icon (widget,
                                   stock_id,
                                   stock_size, NULL);
  g_return_if_fail(pixbuf != NULL);
}

void
GtkCXX::CellRendererPopup::render (GdkWindow            *window,
                                   GtkWidget            *widget,
                                   GdkRectangle         *background_area,
                                   GdkRectangle         *cell_area,
                                   GdkRectangle         *expose_area,
                                   GtkCellRendererState  flags)
{
  g_print("CellRendererPopup::render\n");
  auto          cell   = _G(GTK_CELL_RENDERER(g_object));
  auto          dashes = _G(Class::Traits::cast(g_object));
  GtkStyle*     style  = gtk_widget_get_style (widget);
  GdkRectangle  toggle_rect;
  GdkRectangle  draw_rect;
  GtkStateType  state;
  gint          xpad, ypad;
  cairo_t*      cr;
  gint          width;
  gint          x, y;

  get_size (widget, cell_area,
            &toggle_rect.x, &toggle_rect.y, &toggle_rect.width, &toggle_rect.height);
  cell[gtk_cell_renderer_get_padding] (&xpad, &ypad);

  toggle_rect.x      += cell_area->x + xpad;
  toggle_rect.y      += cell_area->y + ypad;
  toggle_rect.width  -= xpad * 2;
  toggle_rect.height -= ypad * 2;

  g_print("rect: %d,%d+%d,%d\n", toggle_rect.x, toggle_rect.y, toggle_rect.width, toggle_rect.height);

  if (toggle_rect.width <= 0 || toggle_rect.height <= 0)
    return;
  if (! gtk_cell_renderer_get_sensitive (cell)) {
    state = GTK_STATE_INSENSITIVE;
  } else if ((flags & GTK_CELL_RENDERER_SELECTED) == GTK_CELL_RENDERER_SELECTED) {
    if (gtk_widget_has_focus (widget))
      state = GTK_STATE_SELECTED;
    else
      state = GTK_STATE_ACTIVE;
  } else if ((flags & GTK_CELL_RENDERER_PRELIT) == GTK_CELL_RENDERER_PRELIT &&
             gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT) {
    state = GTK_STATE_PRELIGHT;
  } else {
    if (gtk_widget_is_sensitive (widget))
      state = GTK_STATE_NORMAL;
    else
      state = GTK_STATE_INSENSITIVE;
  }

  if (gdk_rectangle_intersect (expose_area, cell_area, &draw_rect) &&
      (flags & GTK_CELL_RENDERER_PRELIT))
    gtk_paint_shadow (style, window,
                      state, active ? GTK_SHADOW_IN : GTK_SHADOW_OUT,
                      &draw_rect, widget, NULL,
                      toggle_rect.x,     toggle_rect.y,
                      toggle_rect.width, toggle_rect.height);

  toggle_rect.x      += style->xthickness;
  toggle_rect.y      += style->ythickness;
  toggle_rect.width  -= style->xthickness * 2;
  toggle_rect.height -= style->ythickness * 2;

  if (gdk_rectangle_intersect (&draw_rect, &toggle_rect, &draw_rect)) {
    cairo_t  *cr = gdk_cairo_create (window);
    gboolean  inconsistent;

    gdk_cairo_rectangle (cr, &draw_rect);
    cairo_clip (cr);

    gdk_cairo_set_source_pixbuf (cr, pixbuf, toggle_rect.x, toggle_rect.y);
    cairo_paint (cr);
    cairo_destroy (cr);
  }
}

gboolean GtkCXX::CellRendererPopup::activate (GdkEvent             *event,
                                              GtkWidget            *widget,
                                              const gchar          *path,
                                              GdkRectangle         *background_area,
                                              GdkRectangle         *cell_area,
                                              GtkCellRendererState  flags)
{
  g_print("CellRendererPopup::activate\n");
  GdkModifierType state = (GdkModifierType)0;

  if (event && ((GdkEventAny *) event)->type == GDK_BUTTON_PRESS)
    state = (GdkModifierType)(((GdkEventButton *) event)->state);

  clicked (G_OBJECT(widget), path, background_area);

  return TRUE;
}

void GtkCXX::CellRendererPopup::clicked (GObject* widget, const gchar* path, GdkRectangle* cell_area)
{
  g_print("CellRendererPopup::clicked\n");
  g_signal_emit (g_object, signals[CLICKED], 0, widget, path, cell_area);
}
/*  public functions  */

GtkCellRenderer *
CellRendererPopupInterface::new_instance () {
  return GTK_CELL_RENDERER(g_object_new (GtkCXX::Class::Traits::get_type(),
      "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
      NULL));
}

CellRendererPopupInterface*
CellRendererPopupInterface::cast(gpointer obj) {
  return dynamic_cast<CellRendererPopupInterface*>(GtkCXX::Class::get_private(obj));
}

bool CellRendererPopupInterface::is_instance(gpointer obj) {
  return GtkCXX::Class::Traits::is_instance(obj);
}

GType gimp_cell_renderer_popup_get_type() { return GtkCXX::Class::Traits::get_type(); }
GtkCellRenderer* gimp_cell_renderer_popup_new() { return CellRendererPopupInterface::new_instance(); }
void
gimp_cell_renderer_popup_clicked(GimpCellRendererPopup* popup, GObject *widget, const gchar* path, GdkRectangle* cell_area)
{
  CellRendererPopupInterface::cast(popup)->clicked(widget, path, cell_area);
}

