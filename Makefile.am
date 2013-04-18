CXXFLAGS = @CXXFLAGS@

lib_LTLIBRARIES = libfox_cairo-1.0.la

libfox_cairo_1_0_la_LIBADD  = $(FOX_LIBS) $(CAIRO_LDFLAGS) $(PANGO_LDFLAGS)

libfox_cairo_1_0_la_LDFLAGS = \
        -version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)    \
	-export-dynamic

libfox_cairo_1_0_la_SOURCES =  FXDCCairo.cpp

AM_CPPFLAGS = -I$(top_srcdir) -I$(top_builddir) $(FOX_CFLAGS) $(CAIRO_CFLAGS) $(PANGO_CFLAGS)

fox_cairoincludedir=$(includedir)/fox_cairo-1.0


fox_cairoinclude_HEADERS =  FXDCCairo.h xincs_cairo.h
noinst_HEADERS = 

noinst_PROGRAMS = dctest dctest2


dctest_CXXFLAGS = @CXXFLAGS@ @FOX_CFLAGS@ @CAIRO_CFLAGS@ @PANGO_CFLAGS@
dctest_LDADD = @FOX_LIBS@ @CAIRO_LDFLAGS@ @PANGO_LDFLAGS@ $(top_builddir)/libfox_cairo-1.0.la
dctest_SOURCES = dctest.cpp

dctest2_CXXFLAGS = @CXXFLAGS@ @FOX_CFLAGS@ @CAIRO_CFLAGS@ @PANGO_CFLAGS@
dctest2_LDADD = @FOX_LIBS@ @CAIRO_LDFLAGS@ @PANGO_LDFLAGS@ $(top_builddir)/libfox_cairo-1.0.la
dctest2_SOURCES = dctest2.cpp

bin_SCRIPTS = fox-cairo-config
