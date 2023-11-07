/* configure.h -- The prototypes of functions used in config.c.  */

/* Copyright (C) 1993-2000, 2006-2007 Free Software Foundation, Inc.

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

#ifndef _GIT_CONFIGURE_H
#define _GIT_CONFIGURE_H


#include "stdc.h"


#define MAXLINE 1024


#define NO_SEEK 0
#define DO_SEEK 1


#define IFS ';'     /* Internal field separator.  */
#define ICS '#'     /* Internal comment separator.  */
#define IAS '='     /* Internal assignment operator.  */


extern int configuration_init PROTO ((char *));
extern void configuration_end PROTO (());
extern int configuration_section PROTO ((char *));
extern void configuration_getvarinfo PROTO ((char *, char **, int, int));


#endif  /* _GIT_CONFIGURE_H */
