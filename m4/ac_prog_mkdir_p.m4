# mkdir_p.m4
# $Id: ac_prog_mkdir_p.m4,v 1.1.2.1 2008/08/09 17:51:48 shelton Exp $
dnl Copyright (C) 2007 Rashid N. "CityCat" Achilov
dnl This file is free software; you will have umlimited permissions
dnl to copy and/or distribute it with or wihtout modifications,
dnl as long, as this notice will retained

AC_DEFUN([AC_PROG_MKDIR_P],
[
  dnl mkdir_p detects mkdir command and their support of "-p" key
  dnl AWK must be detected and "AWK" variable sholud be filled

  AC_CHECK_TOOL(MKDIR, mkdir, none)

  if test $MKDIR = "none"; then
    AC_MSG_ERROR(Base BSD tool mkdir was not found)
  fi

  mkkey=`mkdir 2>&1 | tail -c +15 | head -c 2`

  if test $mkkey = "-p"; then
    MKDIR="$MKDIR $mkkey"
  fi

  AC_SUBST(MKDIR)
])
