ACLOCAL_AMFLAGS = -I m4 --install

bin_PROGRAMS = rover_daemon
rover_daemon_SOURCES = src/main.cpp src/deviceuc0service.h src/deviceuc0service.cpp src/logging.h src/logging.cpp src/messagequeue.h src/messagequeue.th src/netservice.h src/netservice.cpp src/server.h src/server.cpp src/videostreammanager.h src/videostreammanager.cpp src/utils.h
rover_daemon_LDADD = $(DEPS_LIBS)
rover_daemon_CPPFLAGS = -std=c++14 -pthread

EXTRA_DIST = m4/PLACEHOLDER

