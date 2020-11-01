AUTOMAKE_OPTIONS = foreign subdir-objects

bin_PROGRAMS = openzoo
openzoo_SOURCES = \
	src/editor.cpp \
	src/elements.cpp \
	src/game.cpp \
	src/high_scores.cpp \
	src/input.cpp \
	src/oop.cpp \
	src/sounds.cpp \
	src/txtwind.cpp \
	src/utils.cpp \
	src/video.cpp \
	src/driver_sdl.cpp \
	src/audio_simulator.cpp \
	src/main.cpp

openzoo_CXXFLAGS = $(SDL2_CFLAGS)
openzoo_LDFLAGS = $(SDL2_LIBS)
