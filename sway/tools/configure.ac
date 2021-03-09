AC_PREREQ(2.61)
AC_INIT(arping, 0.01, thomas@habets.se)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/swaylib.cc])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)

AC_LANG(C++)
AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_INSTALL

#AC_CHECK_LIB([simdjson], [simdjson::builtin_implementation])

LDFLAGS+=" -lsimdjson"
CXXFLAGS+=" -Os -mtune=native -march=native"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C++ Compiler...: $CXX $CXXFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
