/* signal.h -- Function prototypes for those stupid functions in signal.c.  */

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

#ifndef _GIT_SIGNAL_H
#define _GIT_SIGNAL_H


#include <signal.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include "stdc.h"


extern int user_heart_attack;


extern void signals_init PROTO (());
extern void signal_handlers PROTO ((int));
extern void signals PROTO ((int));
extern void service_pending_signals PROTO (());


#endif  /* _GIT_SIGNAL_H */
