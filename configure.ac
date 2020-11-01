AC_INIT([OpenZoo], [0.1.0], [kontakt+openzoo@asie.pl])
AM_INIT_AUTOMAKE([foreign])

AC_PROG_CXX

AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([dirent.h sys/stat.h])
AC_LANG_POP

AC_ARG_WITH([driver],
	AS_HELP_STRING([--with-driver=DRIVER], [Use specified driver]))

AS_IF([test "x$with_driver" = "x"], [
	dnl Pick driver.
	with_driver=sdl2
])

AS_CASE([$with_driver],
	[sdl2], [
		PKG_CHECK_MODULES(SDL2, sdl2 >= 2.0.0)
		AM_CONDITIONAL([DRIVER_SDL2], [true])
		AC_SUBST([DRIVER_CFLAGS], [$SDL2_CFLAGS])
		AC_SUBST([DRIVER_LDFLAGS], [$SDL2_LIBS])
	],
	[AC_MSG_ERROR("driver not selected or invalid driver")]
)

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
