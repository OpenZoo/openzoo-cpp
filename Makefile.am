AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = openzoo
openzoo_SOURCES = \
	src/assets.cpp \
	src/driver.cpp \
	src/editor.cpp \
	src/elements.cpp \
	src/file_selector.cpp \
	src/filesystem.cpp \
	src/game.cpp \
	src/high_scores.cpp \
	src/oop.cpp \
	src/sounds.cpp \
	src/txtwind.cpp \
	src/user_interface.cpp \
	src/user_interface_slim.cpp \
	src/utils/iostream.cpp \
	src/utils/math.cpp \
	src/utils/strings.cpp \
	src/world_serializer.cpp

if DRIVER_SDL2
openzoo_SOURCES += \
	src/driver_sdl2.cpp \
	src/filesystem_posix.cpp \
	src/audio_simulator.cpp \
	src/audio_simulator_bandlimited.cpp
endif

if DRIVER_NULL
openzoo_SOURCES += src/driver_null.cpp
endif

if DRIVER_MSDOS
openzoo_SOURCES += \
	src/driver_msdos.cpp \
	src/filesystem_msdos.cpp \
	src/filesystem_posix.cpp
endif

if USE_CXX_SHIM
openzoo_SOURCES += src/cxx_shim.cpp
endif

openzoo_CFLAGS = $(DRIVER_CFLAGS) -DLABEL_CACHE
openzoo_CXXFLAGS = $(DRIVER_CFLAGS) -DLABEL_CACHE
openzoo_LDADD = $(DRIVER_LDFLAGS)
