ACLOCAL_AMFLAGS = -I m4 --install

lib_LTLIBRARIES = librover.la
librover_la_SOURCES = src/types.h src/uc.h src/uc.c
librover_la_LDFLAGS = -version-info 0:0:0
include_HEADERS = src/types.h src/uc.h

if !SIMBUILD
    librover_la_CPPFLAGS = -I@KDIR@/include
endif

pkgcfgdir = $(prefix)/lib/pkgconfig
pkgcfg_DATA = librover.pc
DISTCLEANFILES = librover.pc librover.pc.in librover-uninstalled.pc librover-uninstalled.sh

EXTRA_DIST = m4/PLACEHOLDER

