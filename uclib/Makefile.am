ACLOCAL_AMFLAGS = -I m4 --install

lib_LTLIBRARIES = librover.la
librover_la_SOURCES = src/types.h src/uc.h src/uc.c
librover_la_LDFLAGS = -version-info 0:0:0
include_HEADERS = src/uc.h

librover_la_CFLAGS = -I@KDIR@/include/misc

pkgcfgdir = $(prefix)/lib/pkgconfig
pkgcfg_DATA = librover.pc
DISTCLEANFILES = librover.pc librover.pc.in librover-uninstalled.pc librover-uninstalled.sh

bin_PROGRAMS = roveruclibtest
roveruclibtest_SOURCES = src/roveruclibtest.c
roveruclibtest_LDADD = $(lib_LTLIBRARIES)

roveruclibtest_CFLAGS = -I./src


EXTRA_DIST = m4/PLACEHOLDER

