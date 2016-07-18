ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = rover_daemon
rover_daemon_SOURCES = src/main.cpp src/deviceuc0service.cpp src/logging.cpp src/messagequeue.cpp src/netservice.cpp src/server.cpp src/videostreammanager.cpp
rover_daemon_LDADD = $(DEPS_LIBS)
rover_daemon_CPPFLAGS = -std=c++14 -pthread

EXTRA_DIST = m4/PLACEHOLDER

