/* xio.c -- Safe versions of the system io primitives.  It also includes a
   new version of the readlink system call that computes the number of
   characters required to hold the entire link.  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifndef HAVE_GETCWD
#include <sys/param.h>
#ifdef MAXPATHLEN
#define MAXPATHSIZE MAXPATHLEN
#else
#ifdef PATH_MAX
#define MAXPATHSIZE PATH_MAX
#else
#define MAXPATHSIZE 1024
#endif /* PATH_MAX */
#endif /* MAXPATHLEN */
#endif /* HAVE_GETCWD */

#include <errno.h>

/* Not all systems declare ERRNO in errno.h... and some systems #define it! */
#if !defined (errno)
extern int errno;
#endif /* !errno */


#include "xmalloc.h"
#include "xstring.h"
#include "xio.h"


int
xread(fd, buf, count)
    int fd;
    char *buf;
    size_t count;
{
    int chars;
    int old_errno = errno;

    if (count <= 0)
	return count;

#ifdef EINTR
    do
    {
	errno = old_errno;
	chars = read(fd, buf, count);
    }
    while (chars < 0 && errno == EINTR);

    return chars;
#else
    return read(fd, buf, count);
#endif
}


int
xwrite(fd, buf, count)
    int fd;
    const char *buf;
    size_t count;
{
    int chars;
    int old_errno = errno;

    if (count <= 0)
	return count;

#ifdef EINTR
    do
    {
	errno = old_errno;
	chars = write(fd, buf, count);
    }
    while (chars < 0 && errno == EINTR);

    return chars;
#else
    return write(fd, buf, count);
#endif
}



int
__xreadlink(path, buf, size)
    const char *path;
    char *buf;
    size_t size;
{
    int chars;
    int old_errno = errno;

    if (size <= 0)
	return size;

#ifdef EINTR
    do
    {
	errno = old_errno;
	chars = readlink(path, buf, size);
    }
    while (chars < 0 && errno == EINTR);

    return chars;
#else
    return read(fd, buf, count);
#endif
}


int
xreadlink(filename)
    const char *filename;
{
    int size = 100;

    for (;;)
    {
	char *buffer = xmalloc(size);
	int nchars = __xreadlink(filename, buffer, (size_t)size);

	if (nchars < size)
	{
	    xfree(buffer);
	    return nchars;
	}

	xfree(buffer);
	size *= 2;
    }
}


int
xfstat(filedes, buf)
    int filedes;
    struct stat *buf;
{
    int result;
    int old_errno = errno;

    do
    {
	errno = old_errno;
	result = fstat(filedes, buf);
    }
    while (result < 0 && errno == EINTR);

    return result;
}


int
xstat(filename, buf)
    const char *filename;
    struct stat *buf;
{
    int result;
    int old_errno = errno;

    do
    {
	errno = old_errno;
	result = stat(filename, buf);
    }
    while (result < 0 && errno == EINTR);

    return result;
}


int
xlstat(filename, buf)
    const char *filename;
    struct stat *buf;
{
    int result;
    int old_errno = errno;

    do
    {
	errno = old_errno;
#ifdef HAVE_LSTAT
	result = lstat(filename, buf);
#else
	result = stat(filename, buf);
#endif /* HAVE_LSTAT */
    }
    while (result < 0 && errno == EINTR);

    return result;
}


char *
xgetcwd()
{
    char *result;
    char *cwd;

    errno = 0;

#ifdef HAVE_GETCWD
    {
	size_t size;

	cwd = xmalloc(size = 64);

	while ((result = getcwd(cwd, size)) == NULL && errno == ERANGE)
	{
	    cwd = xrealloc(cwd, size += 64);
	    errno = 0;
	}
    }
#else
    {
	/* No getcwd() -> getwd(): BSD style... bleah!  */

	cwd    = xmalloc(MAXPATHSIZE + 2);
	result = getwd(cwd);

	if (result)
	    cwd = xrealloc(cwd, strlen(cwd) + 1);
    }
#endif  /* HAVE_GETCWD */

    if (result == NULL)
    {
	xfree(cwd);
	return NULL;
    }

    return cwd;
}


char *
xdirname(name)
    char *name;
{
    char *ptr = strrchr(name, '/');

    if (ptr == NULL)
	return xstrdup(".");
    else
    {
	char *ptr2 = xstrdup(name);
	ptr2[ptr - name] = '0';
	return ptr2;
    }
}


char *
xbasename(name)
    char *name;
{
    char *base;
    size_t len = strlen(name);

    if (name[len - 1] == '/')
	name[len - 1] = '\0';

    base = strrchr(name, '/');
    return base ? base + 1 : name;
}
