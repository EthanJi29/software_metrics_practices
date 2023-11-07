/* system.h -- Prototypes and #defines for the stuff in system.c.  */

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

#ifndef _GIT_SYSTEM_H
#define _GIT_SYSTEM_H


#include "stdc.h"

#if HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
#define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
#define WIFEXITED(stat_val) (((stat_val) & 0xFF) == 0)
#endif
#ifndef WIFSIGNALED
#define WIFSIGNALED(stat_val)   ((int)((stat_val) & 0xFF) != 0)
#endif

extern char *stdout_log_name;
extern char *stderr_log_name;
extern char *stdout_log_template;
extern char *stderr_log_template;

extern int start PROTO ((char *, int));
extern void remove_log PROTO (());
extern void display_errors PROTO ((char *));

#endif  /* _GIT_SYSTEM_H */
