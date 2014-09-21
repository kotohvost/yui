#!/bin/sh
# $Id: autogen.sh,v 1.1.2.2 2008/08/09 17:51:48 shelton Exp $

env AUTOMAKE_VERSION=19 aclocal -I m4
env AUTOCONF_VERSION=259 autoconf
