dnl === configure.ac --------------------------------------------------------===
dnl SECTION 1: Initialization & Setup
dnl SECTION 2: Architecture, target, and host checks
dnl SECTION 3: Command line arguments for the configure script.
dnl SECTION 4: Check for programs we need and that they are the right version
dnl SECTION 5: Check for libraries
dnl SECTION 6: Check for header files
dnl SECTION 7: Check for types and structures
dnl SECTION 8: Check for specific functions needed
dnl SECTION 9: Additional checks, variables, etc.
dnl SECTION 10: Specify the output files and generate it
dnl
dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 1: Initialization & Setup
dnl===
dnl===-----------------------------------------------------------------------===
dnl Initialize autoconf and define the package name, version number and
dnl email address for reporting bugs.
AC_INIT([Z3],[0.1],[gael.thomas@lip6.fr])

dnl Provide a copyright substitution and ensure the copyright notice is included
dnl in the output of --version option of the generated configure script.
AC_SUBST(Z3VM_COPYRIGHT,["Copyright (c) 2011-2011 Université Pierre et Marie Curie."])
AC_COPYRIGHT([Copyright (c) 2011-2011 Université Pierre et Marie Curie.])

dnl Indicate that we require autoconf 2.59 or later. Ths is needed because we
dnl use some autoconf macros only available in 2.59.
AC_PREREQ(2.59)

dnl Verify that the source directory is valid. This makes sure that we are
dnl not configuring some other package (it validates --srcdir argument)
AC_CONFIG_SRCDIR([bootstrap.sh])

dnl Quit if the source directory has already been configured.
dnl NOTE: This relies upon undocumented autoconf behavior.
dnl if test ${srcdir} != "." ; then
dnl   if test -f ${srcdir}/include/vmkit/config.h ; then
dnl     AC_MSG_ERROR([Already configured in ${srcdir}])
dnl   fi
dnl fi

dnl Place all of the extra autoconf files into the config subdirectory. Tell
dnl various tools where the m4 autoconf macros are.
AC_CONFIG_AUX_DIR([./autoconf])


dnl Tell autoconf that this is an LLVM project being configured
dnl This provides the --with-llvmsrc and --with-llvmobj options
dnl LLVM_CONFIG_PROJECT($LLVM_SRC_ROOT,$LLVM_OBJ_ROOT)


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 2: Architecture, target, and host checks
dnl===
dnl===-----------------------------------------------------------------------===

AC_CANONICAL_TARGET
dnl AM_INIT_AUTOMAKE


dnl Check the target for which we're compiling and the host that will do the
dnl compilations. This will tell us which LLVM compiler will be used for
dnl compiling SSA into object code. This needs to be done early because
dnl following tests depend on it.

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 3: Command line arguments for the configure script.
dnl===
dnl===-----------------------------------------------------------------------===

AC_ARG_WITH([vmkit-src],
	   [AS_HELP_STRING(--with-vmkit-src=src_dir_path,
           [VMKit source path])],
       	   [vmkitsrc=$withval],
       	   [AC_MSG_ERROR([[VMKit path not set, please use --with-vmkit-src option]])]
)

AC_CHECK_FILES([${vmkitsrc}]
               [${vmkitsrc}/Makefile.common], ,
	       AC_MSG_ERROR([[Invalid VMKit path]])
)

AC_ARG_WITH([vmkit-obj],
	   [AS_HELP_STRING(--with-vmkit-obj=obj_dir_path [default is vmkit-src path],
           [VMKit object path])],
       	   [vmkitobj=$withval],
       	   [vmkitobj=${vmkitsrc}])

AC_SUBST([vmkitsrc])
AC_SUBST([vmkitobj])

AC_ARG_ENABLE(debug,
   AS_HELP_STRING(--enable-debug,[Build with debug mode (default is NO)]),,enableval=no)

if test ${enableval} = "yes" ; then
  AC_DEFINE([DO_DEBUG], [1], [debug mode])
fi

AC_ARG_ENABLE(gctorture,
   AS_HELP_STRING(--enable-gctorture,[Build with GC torture mode (default is NO)]),,enableval=no)

if test ${enableval} = "yes" ; then
  AC_DEFINE([GC_TORTURE], [1], [gctorture mode])
fi


dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 4: Check for programs we need and that they are the right version
dnl===
dnl===-----------------------------------------------------------------------===

dnl Check for compilation tools
AC_PROG_CPP


dnl Find the install program
AC_PROG_INSTALL

AC_SUBST(SHLIBEXT,$libltdl_cv_shlibext)

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 5: Check for libraries
dnl===
dnl===-----------------------------------------------------------------------===

AC_DEFUN([MY_CHECK_LIB],
[save_LDFLAGS="$LDFLAGS"
LDFLAGS="$LDFLAGS $3"
AC_CHECK_LIB([$1], [$2], [$4], [$5])
LDFLAGS="$save_LDFLAGS"
])

dnl Propagate the shared library extension that the libltdl checks did to
dnl the Makefiles so we can use it there too

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 6: Check for header files
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 7: Check for types and structures
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 8: Check for specific functions needed
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 9: Additional checks, variables, etc.
dnl===
dnl===-----------------------------------------------------------------------===

dnl===-----------------------------------------------------------------------===
dnl===
dnl=== SECTION 10: Specify the output files and generate it
dnl===
dnl===-----------------------------------------------------------------------===

dnl Configure header files
dnl WARNING: dnl If you add or remove any of the following config headers, then
dnl you MUST also update Makefile.rules so that the variable FilesToConfig
dnl contains the same list of files as AC_CONFIG_HEADERS below. This ensures the
dnl files can be updated automatically when their *.in sources change.
AC_CONFIG_HEADERS([lib/config.h])

AC_CONFIG_FILES([Makefile.config])

AC_OUTPUT
