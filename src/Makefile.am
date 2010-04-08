bin_PROGRAMS = sysjs

INCLUDES = -I$(top_srcdir)/include

noinst_HEADERS = jsdso.h

AM_CXXFLAGS = ${MOZJS_CFLAGS} ${XULU_CFLAGS} -DXP_UNIX -DJS_HAS_DSO_OBJECT

sysjs_SOURCES = js.cpp jsdso.cpp
sysjs_LDADD = ${MOZJS_LIBS}

