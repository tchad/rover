ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = rover_daemon
rover_daemon_SOURCES = src/main.cpp
rover_daemon_LDADD = $(DEPS_LIBS)
rover_daemon_CPPFLAGS = -std=c++14

EXTRA_DIST = m4/PLACEHOLDER

