/* stat.h -- Safely defined stat constants.  */

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

#ifndef _GIT_STAT_H
#define _GIT_STAT_H


#include <fcntl.h>
#include <sys/stat.h>


/* Ugly ... */

/*
 * If the macros defined in sys/stat.h do not work properly, undefine them.
 * We will define them later ...
 * - Tektronix UTekV
 * - Amdahl UTS
 * - Motorola System V/88
 */

#ifndef S_IFMT
#define S_IFMT          00170000
#endif

#ifndef S_IFSOCK
#define S_IFSOCK        0140000
#endif

#ifndef S_IFLNK
#define S_IFLNK         0120000
#endif

#ifndef S_IFREG
#define S_IFREG         0100000
#endif

#ifndef S_IFBLK
#define S_IFBLK         0060000
#endif

#ifndef S_IFDIR
#define S_IFDIR         0040000
#endif

#ifndef S_IFCHR
#define S_IFCHR         0020000
#endif

#ifndef S_IFIFO
#define S_IFIFO         0010000
#endif

#ifndef S_ISUID
#define S_ISUID         0004000
#endif

#ifndef S_ISGID
#define S_ISGID         0002000
#endif

#ifndef S_ISVTX
#define S_ISVTX         0001000
#endif


#ifdef STAT_MACROS_BROKEN
#undef S_ISBLK
#undef S_ISCHR
#undef S_ISDIR
#undef S_ISFIFO
#undef S_ISLNK
#undef S_ISMPB
#undef S_ISMPC
#undef S_ISNWK
#undef S_ISREG
#undef S_ISSOCK
#endif /* STAT_MACROS_BROKEN */


#ifndef S_ISLNK
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK)
#endif

#ifndef S_ISREG
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR)
#endif

#ifndef S_ISCHR
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR)
#endif

#ifndef S_ISBLK
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK)
#endif

#ifndef S_ISFIFO
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO)
#endif

#ifndef S_ISSOCK
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK)
#endif

#ifndef S_IRWXU
#define S_IRWXU         0000700
#endif

#ifndef S_IRWXG
#define S_IRWXG         0000070
#endif

#ifndef S_IRWXO
#define S_IRWXO         0000007
#endif

/* Finally ... :-( */


#endif  /* _GIT_STAT_H */
