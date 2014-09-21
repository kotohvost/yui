# gawk.m4
# $Id: ac_prog_gawk.m4,v 1.1.2.1 2008/08/09 17:51:48 shelton Exp $
dnl Copyright (C) 2007 Rashid N. "CityCat" Achilov
dnl This file is free software; you will have umlimited permissions
dnl to copy and/or distribute it with or wihtout modifications,
dnl as long, as this notice will retained

AC_DEFUN([AC_PROG_GAWK],
[
  dnl gawk.m4 script for checking on GNU AWK presence at set full path to it

  AC_PATH_TOOL(AWK, gawk, none)

  if test $AWK = "none"; then
    AC_MSG_ERROR(GNU AWK did not found, install it first)
  fi

  AC_SUBST(AWK)
])
