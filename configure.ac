AC_INIT
AC_CONFIG_SRCDIR([configure.ac])

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(dict2, 2.0.1)
AM_ICONV

AC_LANG([C])
AC_PROG_CC
LT_INIT

# Checks for libraries.
PKG_CHECK_MODULES(GTK, libglade-2.0 libgnomeui-2.0 >= 1.110.0)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

CFLAGS="$CFLAGS $GTK_CFLAGS"
LDFLAGS="$LDFLAGS -export-dynamic"

if test "${prefix}" = "NONE"; then
   PREF="/usr/local"
else
   PREF="${prefix}"
fi

AC_DEFINE_UNQUOTED(INSTALL_PREFIX, "${PREF}", [The installation prefix.])

AC_CONFIG_FILES([Makefile src/Makefile data/Makefile docs/Makefile])
AC_OUTPUT
