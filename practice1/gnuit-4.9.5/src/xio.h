/* xio.h -- Function prototypes used in xio.c.  */

/* Copyright (C) 1993-1999, 2006-2007 Free Software Foundation, Inc.

 This file is part of gnuit.

 gnuit is free software: you can redistribute it and/or modify it
 under the terms of the GNU General Public License as published
 by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.

 gnuit is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public
 License along with this program. If not, see
 http://www.gnu.org/licenses/. */

/* Written by Tudor Hulubei and Andrei Pitis.  */

#ifndef _GIT_XIO_H
#define _GIT_XIO_H


#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include "stat.h"
#include "stdc.h"


extern int xread PROTO ((int, char *, size_t));
extern int xwrite PROTO ((int, const char *, size_t));

#ifndef HAVE_RENAME
extern int rename PROTO ((const char *, const char *));
#endif /* HAVE_RENAME */

#ifndef HAVE_READLINK
extern int readlink PROTO ((const char *, char *, size_t));
#endif /* HAVE_READLINK */

extern int xreadlink PROTO ((const char *));

extern int xfstat PROTO ((int, struct stat *));
extern int xstat PROTO ((const char *, struct stat *));
extern int xlstat PROTO ((const char *, struct stat *));

extern char *xgetcwd PROTO (());
extern char *xdirname PROTO ((char *));
extern char *xbasename PROTO ((char *));


#endif /* _GIT_XIO_H */
