# homedir.m4
# $Id: ac_homedir.m4,v 1.1.2.1 2008/08/09 17:51:48 shelton Exp $
dnl Copyright (C) 2007 Rashid N. "CityCat" Achilov
dnl This file is free software; you will have umlimited permissions
dnl to copy and/or distribute it with or wihtout modifications,
dnl as long, as this notice will retained

AC_DEFUN([AC_HOMEDIR],
[
  dnl Checking for homedir specification (developer option)
  AC_MSG_CHECKING(for homedir)
  AC_ARG_WITH(homedir,
        [  --with-homedir[=DIR]      developer mode: set homedir to backup [[none]]],
        [if test x$withval != xyes; then
              homedir=$withval
           else
              homedir="./"
         fi],[homedir="./"])

  if test "$homedir" != "./"; then
    AC_MSG_RESULT(yes)
    AC_MSG_CHECKING(for homedir value)
    AC_MSG_RESULT($homedir)
   else
     AC_MSG_RESULT(no)
  fi

  AC_SUBST(homedir)
])
