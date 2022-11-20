project('openzoo', ['c', 'cpp'])
cc = meson.get_compiler('c')

openzoo_config = configuration_data()

openzoo_sources = [
	'src/assets.cpp',
	'src/driver.cpp',
	'src/editor.cpp',
	'src/elements.cpp',
	'src/file_selector.cpp',
	'src/filesystem.cpp',
	'src/game.cpp',
	'src/high_scores.cpp',
	'src/oop.cpp',
	'src/sounds.cpp',
	'src/txtwind.cpp',
	'src/user_interface.cpp',
	'src/user_interface_slim.cpp',
	'src/user_interface_super_zzt.cpp',
	'src/utils/iostream.cpp',
	'src/utils/mathutils.cpp',	
	'src/utils/stringutils.cpp',
	'src/world_serializer.cpp'
]

openzoo_incdirs = ['src']

openzoo_dependencies = [
]

driver = get_option('driver')
if driver == 'sdl2'
	openzoo_dependencies += dependency('sdl2')
	openzoo_sources += [
		'src/driver_sdl2.cpp',
		'src/filesystem_posix.cpp',
		'src/audio_simulator.cpp',
		'src/audio_simulator_bandlimited.cpp'
	]
elif driver == 'msdos'
	openzoo_sources += [
		'src/driver_msdos.cpp'
	]
else
	openzoo_sources += [
		'src/driver_null.cpp'
	]
endif

if cc.has_function('getcwd')
	openzoo_config.set('HAVE_GETCWD', 1)
endif
if cc.has_function('qsort')
	openzoo_config.set('HAVE_QSORT', 1)
endif
if cc.has_function('realpath')
	openzoo_config.set('HAVE_REALPATH', 1)
endif
if cc.has_function('strnlen')
	openzoo_config.set('HAVE_STRNLEN', 1)
endif
if cc.has_function('vsniprintf')
	openzoo_config.set('HAVE_VSNIPRINTF', 1)
endif

configure_file(output: 'config.h',
               configuration: openzoo_config)

executable('openzoo', openzoo_sources,
	include_directories: include_directories(openzoo_incdirs),
	dependencies: openzoo_dependencies)
