# cv2cl.m4
# $Id: ac_prog_cvs2cl_pl.m4,v 1.1.2.1 2008/08/09 17:51:48 shelton Exp $
dnl Copyright (C) 2007 Rashid N. "CityCat" Achilov
dnl This file is free software; you will have umlimited permissions
dnl to copy and/or distribute it with or wihtout modifications,
dnl as long, as this notice will retained

AC_DEFUN([AC_PROG_CVS2CL_PL],
[
  dnl cvs2cl.pl script to parse 'cvs log' output and generate ChangeLog

  AC_CHECK_TOOL(CVS2CL, cvs2cl.pl, $ac_aux_dir/cvs2cl.pl)

  if test $CVS2CL = "$ac_aux_dir/cvs2cl.pl"; then
    cvs2sub="../$CVS2CL"
   else
     cvs2sub="$CVS2CL"
  fi

  AC_SUBST(cvs2sub)
])
