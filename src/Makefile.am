bin_PROGRAMS = sysjs

INCLUDES = -I$(top_srcdir)/include

noinst_HEADERS = jsdso.h js_sysutil.h

AM_CXXFLAGS =  ${INCLUDES} ${MOZJS_CFLAGS} ${XULU_CFLAGS} -DXP_UNIX -DJS_HAS_DSO_OBJECT

sysjs_SOURCES = js.cpp jsdso.cpp js_sysutil.cpp
sysjs_LDADD = ${MOZJS_LIBS}

