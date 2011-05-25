bin_PROGRAMS = sysjs

INCLUDES = -I$(top_srcdir)/include

noinst_HEADERS = jsdso.h js_sysutil.h

AM_CXXFLAGS =  ${INCLUDES} ${MOZJS_CFLAGS} ${XULU_CFLAGS} -DXP_UNIX -DJS_HAS_DSO_OBJECT -Wall -DEXPORT_JS_API

sysjs_SOURCES =  jsdso.cpp js_sysutil.cpp js_socket.cpp js.cpp
sysjs_LDADD = ${MOZJS_LIBS}

