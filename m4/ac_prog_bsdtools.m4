# bsdtools.m4
# $Id: ac_prog_bsdtools.m4,v 1.1.2.1 2008/08/09 17:51:48 shelton Exp $
dnl Copyright (C) 2007 Rashid N. "CityCat" Achilov
dnl This file is free software; you will have umlimited permissions
dnl to copy and/or distribute it with or wihtout modifications,
dnl as long, as this notice will retained

AC_DEFUN([AC_PROG_BSDTOOLS],
[
  dnl Common BSD tools, ordinary used in Makefiles

  AC_CHECK_TOOL(RM, rm, [none])

  if test $RM = "none"; then
    AC_MSG_ERROR(Base BSD tool rm is missed)
  fi

  AC_CHECK_TOOL(STRIP, strip, [none])

  if test $STRIP = "none"; then
    AC_MSG_ERROR(Base BSD tool strip is missed)
  fi

  AC_CHECK_TOOL(TAR, tar, [none])

  if test $TAR = "none"; then
    AC_MSG_ERROR(Base BSD tool tar is missed)
  fi

  AC_CHECK_TOOL(MV, mv, [none])

  if test $MV = "none"; then
    AC_MSG_ERROR(Base BSD tool mv is missed)
  fi

  AC_CHECK_TOOL(CP, cp, [none])

  if test $CP = "none"; then
    AC_MSG_ERROR(Base BSD tool cp is missed)
  fi

  AC_CHECK_TOOL(CHMOD, chmod, [none])

  if test $CHMOD = "none"; then
    AC_MSG_ERROR(Base BSD tool chmod is missed)
  fi

  AC_CHECK_TOOL(SYSCTL, sysctl, [none])

  if test $SYSCTL = "none"; then
    AC_MSG_ERROR(Base BSD tool sysctl is missed)
  fi
])
