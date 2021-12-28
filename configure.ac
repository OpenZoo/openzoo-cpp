AC_INIT([OpenZoo], [0.1.0], [kontakt+openzoo@asie.pl])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_LANG_PUSH([C++])
AX_CXX_COMPILE_STDCXX(17)

AC_CANONICAL_HOST
AC_CHECK_HEADERS([dirent.h sys/stat.h])

AC_ARG_WITH([driver],
	AS_HELP_STRING([--with-driver=DRIVER], [Use specified driver]))

AS_IF([test "x$with_driver" = "x"], [
	dnl Pick driver.
	with_driver=sdl2
])

AS_CASE([$with_driver],
	[null], [],
	[sdl2], [
		PKG_CHECK_MODULES(SDL2, sdl2 >= 2.0.0)
		AC_SUBST(DRIVER_CFLAGS)
		AC_SUBST(DRIVER_LDFLAGS)
		case "${host_os}" in
			mingw*)
				DRIVER_CFLAGS="-mwindows $SDL2_CFLAGS"
				DRIVER_LDFLAGS="-Wl,-Bstatic -lstdc++ -lpthread -lmingw32 -lSDL2main -Wl,-Bdynamic $SDL2_LIBS -static-libgcc -static-libstdc++"
				;;
			*)
				DRIVER_CFLAGS="$SDL2_CFLAGS"
				DRIVER_LDFLAGS="$SDL2_LIBS"
				;;
		esac
	],
	[msdos], [],
	[AC_MSG_ERROR("driver not selected or invalid driver")]
)

AM_CONDITIONAL([DRIVER_NULL], [test "x$with_driver" = "xnull"])
AM_CONDITIONAL([DRIVER_SDL2], [test "x$with_driver" = "xsdl2"])
AM_CONDITIONAL([DRIVER_MSDOS], [test "x$with_driver" = "xmsdos"])

dnl Using vsniprintf means we can avoid linking in as much floating-point code.
dnl However, it seems to be newlib-specific.
AC_CHECK_FUNCS([vsniprintf])

AC_CHECK_FUNCS([strnlen])

dnl filesystem_posix.cpp
AC_CHECK_FUNCS([getcwd realpath])

dnl txtwind.cpp
AC_CHECK_FUNCS([qsort])

AX_CHECK_COMPILE_FLAG([-fno-exceptions], [AX_APPEND_FLAG([-fno-exceptions])], [])
AX_CHECK_COMPILE_FLAG([-fno-rtti], [AX_APPEND_FLAG([-fno-rtti])], [])
AX_CHECK_COMPILE_FLAG([-fno-unwind-tables], [AX_APPEND_FLAG([-fno-unwind-tables])], [])

AC_LANG_POP

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
