AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = openzoo
openzoo_SOURCES = \
	src/cxx_shim.cpp \
	src/editor.cpp \
	src/elements.cpp \
	src/file_selector.cpp \
	src/game.cpp \
	src/high_scores.cpp \
	src/input.cpp \
	src/oop.cpp \
	src/sounds.cpp \
	src/txtwind.cpp \
	src/utils.cpp \
	src/video.cpp

if DRIVER_SDL2
openzoo_SOURCES += \
	src/driver_sdl2.cpp \
	src/audio_simulator.cpp
endif

if DRIVER_NULL
openzoo_SOURCES += src/driver_null.cpp
endif

if DRIVER_MSDOS
openzoo_SOURCES += src/driver_msdos.cpp
endif

openzoo_CFLAGS = $(DRIVER_CFLAGS)
openzoo_CXXFLAGS = $(DRIVER_CFLAGS)
openzoo_LDFLAGS = $(DRIVER_LDFLAGS)
