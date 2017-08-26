## Process this file with automake to produce Makefile.in

if PLATFORM_OSX
xobjective_c = "-xobjective-c"
endif

AM_CPPFLAGS = \
        -DISO_CODES_LOCATION=\"$(ISO_CODES_LOCATION)\"		\
        -DISO_CODES_LOCALEDIR=\"$(ISO_CODES_LOCALEDIR)\"	\
	-DG_LOG_DOMAIN=\"Gimp-Presets\"

INCLUDES = \
	-I$(top_builddir)		\
	-I$(top_srcdir)			\
	-I$(top_builddir)/app	\
	-I$(top_srcdir)/app		\
	$(GEGL_CFLAGS)			\
	$(GTK_CFLAGS)			\
	$(SOUP_CFLAGS)			\
	-I$(includedir) 		\
	$(xobjective_c)

noinst_LIBRARIES = libapphttpd.a

libapphttpd_a_sources = \
	httpd-enums.h   		\
	httpd.cpp			\
	httpd.h				\
	httpd-features.cpp		\
	httpd-features.h

libapphttpd_a_built_sources = httpd-enums.c

libapphttpd_a_SOURCES = \
	$(libapphttpd_a_built_sources) $(libapphttpd_a_sources)


#
# rules to generate built sources
#
# setup autogeneration dependencies
gen_sources = xgen-wec
CLEANFILES = $(gen_sources)

httpd-enums.c: $(srcdir)/httpd-enums.h $(GIMP_MKENUMS)
	$(GIMP_MKENUMS) \
		--fhead "#include \"config.h\"\n#include <gtk/gtk.h>\n#include \"libgimpbase/gimpbase.h\"\n#include \"httpd-enums.h\"\n#include \"gimp-intl.h\"" \
		--fprod "\n/* enumerations from \"@filename@\" */" \
		--vhead "GType\n@enum_name@_get_type (void)\n{\n  static const G@Type@Value values[] =\n  {" \
		--vprod "    { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
		--vtail "    { 0, NULL, NULL }\n  };\n" \
		--dhead "  static const Gimp@Type@Desc descs[] =\n  {" \
		--dprod "    { @VALUENAME@, @valuedesc@, @valuehelp@ }," \
		--dtail "    { 0, NULL, NULL }\n  };\n\n  static GType type = 0;\n\n  if (G_UNLIKELY (! type))\n    {\n      type = g_@type@_register_static (\"@EnumName@\", values);\n      gimp_type_set_translation_context (type, \"@enumnick@\");\n      gimp_@type@_set_value_descriptions (type, descs);\n    }\n\n  return type;\n}\n" \
		$(srcdir)/httpd-enums.h > xgen-wec \
	&& cp xgen-wec $(@F) \
	&& rm -f xgen-wec
