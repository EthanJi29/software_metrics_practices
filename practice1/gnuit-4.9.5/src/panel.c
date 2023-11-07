/* panel.c -- The panels management file.  */

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#else /* !HAVE_STDLIB_H */
#include "ansi_stdlib.h"
#endif /* !HAVE_STDLIB_H */

#include <sys/types.h>


#if HAVE_INTTYPES_H
# include <inttypes.h>
#endif
#if HAVE_STDINT_H
# include <stdint.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef UINTMAX_MAX
# define UINTMAX_MAX ((uintmax_t) -1)
#endif



#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <ctype.h>
#include "file.h"

#ifdef HAVE_VALUES_H
#include <values.h>
#endif /* HAVE_VALUES_H */

#include <limits.h>

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif /* INT_MAX */

#ifdef HAVE_64BIT_IO
#ifndef LONG_LONG_MAX
#define LONG_LONG_MAX 9223372036854775807LL
#endif /* LONG_LONG_MAX */
#endif /* HAVE_64BIT_IO */

#ifdef HAVE_64BIT_IO
#define MAXFILESIZE LONG_LONG_MAX
#else /* !HAVE_64BIT_IO */
#define MAXFILESIZE INT_MAX
#endif /* !HAVE_64BIT_IO */

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include "xtime.h"

#include <errno.h>

/* Not all systems declare ERRNO in errno.h... and some systems #define it! */
#if !defined (errno)
extern int errno;
#endif /* !errno */

/* Get the statfs() prototipe from sys/vfs.h.  */
#ifdef HAVE_LINUX
#include <sys/vfs.h>
#endif  /* HAVE_LINUX */

#include <assert.h>

#include <human.h>

#include "stdc.h"
#include "xstring.h"
#include "xmalloc.h"
#include "xio.h"
#include "fsusage.h"
#include "window.h"
#include "status.h"
#include "signals.h"
#include "tty.h"
#include "inputline.h"
#include "panel.h"
#include "tilde.h"
#include "fnmatch.h"
#include "configure.h"
#include "system.h"
#include "misc.h"
#include "stat.h"

/* prototypes from gnulib idcache.c (no idcache.h for some reason) */
char *getuser  PROTO ((uid_t uid));
char *getgroup PROTO ((gid_t gid));

extern int AnsiColors;
extern int TypeSensitivity;


char rights[16] = "-rwxrwxrwx";


#define FILE_DISPLAY_MODES      7

char *FileDisplayMode[FILE_DISPLAY_MODES] =
{
    "OwnerGroup",
    "DateTime",
    "Size",
    "AbbrevSize",
    "Mode",
    "FullName",
    "All",
};


#define FILE_SORT_METHODS       9

char *FileSortMethod[FILE_SORT_METHODS] =
{
    "Name",
    "Extension",
    "Size",
    "Date",
    "Mode",
    "OwnerId",
    "GroupId",
    "OwnerName",
    "GroupName",
};


#define PANEL_FIELDS    20

static char *PanelFields[PANEL_FIELDS] =
{
    "PanelFrame",
    "PanelBackground",
    "PanelThumbForeground",
    "PanelThumbBackground",
    "PanelThumbBrightness",
    "PanelSelectedFile",
    "PanelSelectedFileBrightness",
    "PanelNotSelectedFile",
    "PanelNotSelectedFileBrightness",
    "PanelCurrentSelectedFile",
    "PanelCurrentNotSelectedFile",
    "PanelCurrentFile",
    "PanelPath",
    "PanelPathBrightness",
    "PanelDeviceFreeSpace",
    "PanelDeviceFreeSpaceBrightness",
    "PanelFileInfo",
    "PanelFileInfoBrightness",
    "PanelFilesInfo",
    "PanelFilesInfoBrightness",
};

#ifdef HAVE_LINUX
static int PanelColors[PANEL_FIELDS] =
{
    WHITE, BLUE, BLACK, WHITE, OFF, YELLOW, ON, WHITE, ON, YELLOW,
    WHITE, CYAN, RED, OFF, RED, OFF, RED, OFF, BLACK, OFF,
};
#else   /* !HAVE_LINUX */
static int PanelColors[PANEL_FIELDS] =
{
    WHITE, BLACK, BLACK, WHITE, OFF, WHITE, ON, WHITE, OFF, WHITE,
    BLACK, WHITE, BLACK, OFF, BLACK, OFF, BLACK, OFF, BLACK, OFF
};
#endif  /* !HAVE_LINUX */

#define PanelFrame                      PanelColors[0]
#define PanelBackground                 PanelColors[1]
#define PanelThumbForeground            PanelColors[2]
#define PanelThumbBackground            PanelColors[3]
#define PanelThumbBrightness            PanelColors[4]
#define PanelSelectedFile               PanelColors[5]
#define PanelSelectedFileBrightness     PanelColors[6]
#define PanelNotSelectedFile            PanelColors[7]
#define PanelNotSelectedFileBrightness  PanelColors[8]
#define PanelCurrentSelectedFile        PanelColors[9]
#define PanelCurrentNotSelectedFile     PanelColors[10]
#define PanelCurrentFile                PanelColors[11]
#define PanelPath                       PanelColors[12]
#define PanelPathBrightness             PanelColors[13]
#define PanelDeviceFreeSpace            PanelColors[14]
#define PanelDeviceFreeSpaceBrightness  PanelColors[15]
#define PanelFileInfo                   PanelColors[16]
#define PanelFileInfoBrightness         PanelColors[17]
#define PanelFilesInfo                  PanelColors[18]
#define PanelFilesInfoBrightness        PanelColors[19]


char *panel_il_message[] =
{
    "Wait, reading directory...",
    "Wait, copying file...",
    "Wait, copying file(s)...",
    "Wait, copying directory...",
    "Wait, deleting directory...",
    "Wait, deleting file(s)...",
    "Wait, moving file...",
    "Wait, moving file(s)...",
    "Wait, moving directory...",
    "Wait, comparing directories...",
    "Wait, comparing files...",
    "Wait, renaming file(s)...",
};

#define PANEL_READ_DIR_MSG      panel_il_message[ 0]
#define PANEL_COPY_FILE_MSG     panel_il_message[ 1]
#define PANEL_COPY_FILES_MSG    panel_il_message[ 2]
#define PANEL_COPY_DIR_MSG      panel_il_message[ 3]
#define PANEL_DELETE_DIR_MSG    panel_il_message[ 4]
#define PANEL_DELETE_FILES_MSG  panel_il_message[ 5]
#define PANEL_MOVE_FILE_MSG     panel_il_message[ 6]
#define PANEL_MOVE_FILES_MSG    panel_il_message[ 7]
#define PANEL_MOVE_DIR_MSG      panel_il_message[ 8]
#define PANEL_COMPARE_DIR_MSG   panel_il_message[ 9]
#define PANEL_COMPARE_FILES_MSG panel_il_message[10]
#define PANEL_RENAME_FILES_MSG  panel_il_message[11]


/* Some useful isearch stack management functions.  */

#define STACK_PUSH(__entry__, __length__)                       \
{                                                               \
    isearch_t current_isearch;                                  \
								\
    current_isearch.entry  = __entry__;                         \
    current_isearch.length = __length__;                        \
								\
    xstack_push(this->isearch_stack, &current_isearch);         \
}


#define STACK_POP(__entry__, __length__)                        \
{                                                               \
    isearch_t tmp_isearch;                                      \
								\
    xstack_pop(this->isearch_stack, &tmp_isearch);              \
								\
    __entry__  = tmp_isearch.entry;                             \
    __length__ = tmp_isearch.length;                            \
}


#define STACK_PREVIEW(__entry__, __length__)                    \
{                                                               \
    isearch_t tmp_isearch;                                      \
								\
    xstack_preview(this->isearch_stack, &tmp_isearch, 1);       \
								\
    __entry__  = tmp_isearch.entry;                             \
    __length__ = tmp_isearch.length;                            \
}


/* Is the current directory the root directory?  */
#define rootdir() (this->path[0] == '/' && this->path[1] == '\0')


static int StartupFileDisplayMode;
static int StartupFileSortMethod;
static int StartupScrollStep;
static int CurrentSortMethod;
static int LeadingDotMatch = OFF;
static int InfoDisplay     = OFF;
static int GroupDigits     = ON;
static int MaxUnscaledDigits=9;

static char nice_try[] = "Nice try, maybe later... :-)";

extern void fatal PROTO ((char *));
extern void panel_update_entry PROTO ((panel_t *, int));
extern char il_read_char PROTO ((char *, char *, int));
extern char *il_read_line PROTO ((char *, char **, char *, xstack_t *));


static xstack_t *copy_history;
static xstack_t *move_history;
static xstack_t *mkdir_history;


static void
xchg(a, b)
    int *a, *b;
{
    int tmp = *a;
    *a = *b;
    *b = tmp;
}


/*
 * Initialize the panel data structures.  Read the necessary data from
 * the configuration files, if needed.
 */
panel_t *
panel_init(path)
    char *path;
{
    static int configured;

    panel_t *this = (panel_t *)xmalloc(sizeof(panel_t));

    this->x		    = 0;
    this->y		    = 0;
    this->lines		    = 1;
    this->columns	    = 1;
    this->focus		    = OFF;
    this->entries	    = 0;
    this->selected_entries  = 0;
    this->last_index	    = -1;
    this->display_mode	    = this->sort_method = 0;
    this->current_entry	    = 0;
    this->first_on_screen   = 0;
    this->on_screen	    = INT_MAX / 2;
    this->temp		    = NULL;
    this->dir		    = NULL;
    this->isearch_stack	    = NULL;
    this->visible	    = 1;
    this->scroll_step       = 1;
    this->thumb		    = 0;
    this->horizontal_offset = 0;

#ifdef HAVE_LINUX
    this->msdosfs = 0;
#endif  /* HAVE_LINUX */

    this->dir_entry = NULL;

    if (chdir(path) == -1)
	fatal("`chdir' failed: permission denied");

    this->path = xgetcwd();

    if (this->path == NULL)
	fatal("`getcwd' failed: permission denied");

    minimize_path(this->path);
    this->pathlen = strlen(this->path);

    this->window = window_init();

    if (configured)
    {
	this->display_mode = StartupFileDisplayMode;
	this->sort_method  = StartupFileSortMethod;
	return this;
    }


    use_section("[Setup]");

    StartupScrollStep = get_int_var("StartupScrollStep", 0);


    use_section("[GITFM-Setup]");

    StartupFileDisplayMode = get_const_var("StartupFileDisplayMode",
					    FileDisplayMode,
					    FILE_DISPLAY_MODES, 0);
    this->display_mode = StartupFileDisplayMode;

    StartupFileSortMethod = get_const_var("StartupFileSortMethod",
					   FileSortMethod,
					   FILE_SORT_METHODS, 0);
    this->sort_method = StartupFileSortMethod;

    MaxUnscaledDigits=get_int_var("MaxUnscaledDigits",9);

    InfoDisplay     = get_flag_var("InfoDisplay",     ON);
    LeadingDotMatch = get_flag_var("LeadingDotMatch", ON);
    GroupDigits     = get_flag_var("GroupDigits", ON);
    

    use_section(AnsiColors ? color_section : monochrome_section);

    get_colorset_var(PanelColors, PanelFields, PANEL_FIELDS);


    copy_history  = xstack_init(sizeof(char *));
    move_history  = xstack_init(sizeof(char *));
    mkdir_history = xstack_init(sizeof(char *));

    configured = 1;
    return this;
}


/*
 * Destroy a panel.
 */
void
panel_end(this)
    panel_t *this;
{
    int i;

    if (this->dir)
	closedir(this->dir);

    for (i = 0; i < this->entries; i++)
	if (this->dir_entry[i].name)
	{
	    xfree(this->dir_entry[i].name);
	    this->dir_entry[i].name = NULL;
	}

    xfree(this->dir_entry);
    xfree(this->temp);

    window_end(this->window);

    xfree(this);
}


window_t *
panel_window(this)
    panel_t *this;
{
    return this->window;
}


/*
 * We might need to call this twice in the same expression, so this is
 * the reason for having tname[2].  If filled != 0, fill the resulting
 * string with spaces up to 14 characters.
 */
static char *
cutname(name, which, filled)
    char *name;
    int which, filled;
{
    static char tname[2][16];

    if (filled)
    {
	memset(tname[which], ' ', 14);
	tname[which][14] = 0;
	return memcpy(tname[which], name, min(strlen(name), 14));
    }
    else
	return strncpy(tname[which], name, 14);
}


static int
sort_compare_fn(_first, _second)
    const void *_first;
    const void *_second;
{
    int retval;
    char *pfirst, *psecond;
    const dir_entry_t *first  = (const dir_entry_t *)_first;
    const dir_entry_t *second = (const dir_entry_t *)_second;
    int first_is_dir  = first->type  == DIR_ENTRY;
    int second_is_dir = second->type == DIR_ENTRY;

    if (first_is_dir != second_is_dir)
	return first_is_dir ? -1 : 1;

    switch (CurrentSortMethod)
    {
	 case SORT_BY_NAME:
	     break;

	 case SORT_BY_EXTENSION:
	     pfirst  = strrchr(first->name,  '.');
	     psecond = strrchr(second->name, '.');

	     if (pfirst && psecond)
	     {
		 retval = strcmp(++pfirst, ++psecond);

		 if (retval != 0)
		     return retval;
	     }
	     else
		 if (pfirst || psecond)
		     return (pfirst ? -1 : 1);

	     break;

	 case SORT_BY_SIZE:
	     if (first->size == second->size)
		 break;
	     return( (first->size > second->size) ? 1 : -1);

	 case SORT_BY_DATE:
	     if (first->mtime == second->mtime)
		 break;
	     return first->mtime - second->mtime;

	 case SORT_BY_MODE:
	     if (first->mode == second->mode)
		 break;
	     return first->mode - second->mode;

	 case SORT_BY_OWNER_ID:
	     if (first->uid == second->uid)
		 break;
	     return first->uid - second->uid;

	 case SORT_BY_GROUP_ID:
	     if (first->gid == second->gid)
		 break;
	     return first->gid - second->gid;

	 case SORT_BY_OWNER_NAME:
	     if (first->uid == second->uid)
		 break;
	     return strcmp(first->owner, second->owner);

	 case SORT_BY_GROUP_NAME:
	     if (first->gid == second->gid)
		 break;
	     return strcmp(first->group, second->group);

	 default:
	     fatal("bad sort method");
    }

    return strcmp(first->name, second->name);
}


void
panel_no_optimizations(this)
    panel_t *this;
{
    this->on_screen = INT_MAX / 2;
}


char *
panel_get_current_file_name(this)
    panel_t *this;
{
    return this->dir_entry[this->current_entry].name;
}


uid_t
panel_get_current_file_uid(this)
    panel_t *this;
{
    return this->dir_entry[this->current_entry].uid;
}


gid_t
panel_get_current_file_gid(this)
    panel_t *this;
{
    return this->dir_entry[this->current_entry].gid;
}


mode_t
panel_get_current_file_mode(this)
    panel_t *this;
{
    return this->dir_entry[this->current_entry].mode;
}


int
panel_get_current_file_type(this)
    panel_t *this;
{
    return this->dir_entry[this->current_entry].type;
}


/*
 * Return an (unconditionally centered) first entry index.
 */

static int
panel_get_centered_fos(this)
    panel_t *this;
{
    int lines = (this->lines - 2);
    int tmp = this->current_entry - (lines / 2);

    if (tmp + lines >= this->entries)
	return max(0, this->entries - lines);
    else
	return max(0, tmp);
}


/*
 * Return the `first on screen' panel entry, i.e. the first file entry
 * in the panel that is visible on the screen.
 */
static int
panel_get_fos(this)
    panel_t *this;
{
    assert(this->first_on_screen >= 0);

    if ((this->first_on_screen <= this->current_entry) &&
	(this->first_on_screen + this->lines - 2 > this->current_entry) &&
	(this->first_on_screen + this->lines - 2 <= this->entries))
	return this->first_on_screen;

    return panel_get_centered_fos(this);
}


static void
panel_set_current_entry(this, entry)
    panel_t *this;
    int entry;
{
    assert(entry >= 0);
    assert(entry < this->entries);

    this->current_entry = entry;
    this->first_on_screen = panel_get_fos(this);
}


void
panel_center_current_entry(this)
    panel_t *this;
{
    this->first_on_screen = panel_get_centered_fos(this);
}


void
panel_activate(this)
    panel_t *this;
{
    this->visible = 1;
}


void
panel_deactivate(this)
    panel_t *this;
{
    this->visible = 0;
}


/*
 * Resize a panel.
 */
void
panel_resize(this, x, y, lines, columns)
    panel_t *this;
    size_t x, y, lines, columns;
{
    this->x = x;
    this->y = y;

    this->lines   = lines;
    this->columns = columns;

    this->thumb = 0;

    if (StartupScrollStep <= 0 || StartupScrollStep >= (this->lines - 1))
	this->scroll_step = this->lines / 2;
    else
	this->scroll_step = StartupScrollStep;

    this->temp = xrealloc(this->temp, this->columns);
    window_resize(this->window, x, y, lines, columns);
    panel_center_current_entry(this);
    this->horizontal_offset = 0;
}


void
panel_set_wrapped_isearch_flag(this, value)
    panel_t *this;
    int value;
{
    this->wrapped_isearch = value;
}


int
panel_isearch_backward(this, string, len, start_entry)
    panel_t *this;
    char *string;
    size_t len;
    int start_entry;
{
    int i;

    for (i = start_entry; i >= 0; i--)
    {
	if (strncasecmp(string, this->dir_entry[i].name, len) == 0)
	{
	    /* Success, return the entry just found.  */
	    return i;
	}
    }

    /* Error, cannot find matching entry.  */
    return -1;
}


int
panel_isearch_forward(this, string, len, start_entry)
    panel_t *this;
    char *string;
    size_t len;
    int start_entry;
{
    int entry;

    for (entry = start_entry; entry < this->entries; entry++)
	if (strncasecmp(string, this->dir_entry[entry].name, len) == 0)
	{
	    /* Success, return the entry just found.  */
	    return entry;
	}

    /* Error, cannot find matching entry.  */
    return -1;
}


#define panel_1s_message il_read_char


char
panel_2s_message(format, string, options, flags)
    char *format;
    char *string;
    char *options;
    int flags;
{
    char c;
    char *message = xmalloc(strlen(format) + strlen(string) + 1);

    sprintf(message, format, string);
    c = panel_1s_message(message, options, flags);
    xfree(message);
    return c;
}


char
panel_3s_message(format, string1, string2, options, flags)
    char *format;
    char *string1;
    char *string2;
    char *options;
    int flags;
{
    char c;
    char *message = xmalloc(strlen(format)+strlen(string1)+strlen(string2)+1);

    sprintf(message, format, string1, string2);
    c = panel_1s_message(message, options, flags);
    xfree(message);
    return c;
}


void
panel_recover(this)
    panel_t *this;
{
    panel_set_current_entry(this, 0);

    panel_2s_message("%s/: Permission denied.",
		     this->path, (char *)NULL,
		     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);

    if (strcmp(this->path, "/") == 0)
	fatal("/: Permission denied");

    strcpy(this->path, "/");
    this->pathlen = 1;
    chdir(this->path);
    panel_action(this, act_REGET, (panel_t *)NULL, (void *)NULL, 1);
}


/*
 * Load the file type information for the given entry.  Choose the
 * longest pattern, such that the order in which the patterns are
 * specified in the configuration file will not matter.
 */
static void
panel_load_fti(this, entry)
    panel_t *this;
    int entry;
{
    size_t length;
    size_t longest_length = 0;
    file_type_info_t *fti;
    file_type_info_t *longest_fti = NULL;

    this->dir_entry[entry].fti_loaded = 1;

    /* FIXME: an even better solution here would be to keep track of
       whether or not the pattern starts with a *.  It is always
       better to match "RMAIL*" than "*.[bg]z".  */
    for (fti = fti_head; fti; fti = fti->next)
	if (fnmatch(fti->pattern, this->dir_entry[entry].name,
		    FNM_PERIOD | FNM_PATHNAME | FNM_CASEFOLD) == 0)
	{
	    length = strlen(fti->pattern);

	    if (length > longest_length)
	    {
		longest_fti = fti;
		longest_length = length;
	    }
	}

    fti = longest_fti;

    if (fti)
    {
	this->dir_entry[entry].brightness =
	    (fti->brightness == -1) ? OFF : fti->brightness;

	this->dir_entry[entry].foreground =
	    (fti->foreground == -1) ? PanelNotSelectedFile : fti->foreground;

	this->dir_entry[entry].background =
	    (fti->background == -1) ? PanelBackground : fti->background;
    }
    else
    {
	/* No match.  An invalid value of the brightness field specifies it.
	   It's ugly, but there are no more free bits in dir_entry_t.  */
	this->dir_entry[entry].brightness = 2;
    }
}


/*
 * Read the inode information.
 */
static void
panel_load_inode(this, entry)
    panel_t *this;
    int entry;
{
    int sz, hour;
    struct stat s;
    struct tm *time;
    char *owner;
    char *group;
    
    memset(&s, 0, sizeof(s));

    xlstat(this->dir_entry[entry].name, &s);

    this->dir_entry[entry].mode = s.st_mode;
    this->dir_entry[entry].uid  = s.st_uid;
    this->dir_entry[entry].gid  = s.st_gid;

    if (s.st_ino)
    {
	if (S_ISDIR(s.st_mode))
	    this->dir_entry[entry].type = DIR_ENTRY;
	else
	{
	    if (S_ISREG(s.st_mode))
	    {
		this->dir_entry[entry].type = FILE_ENTRY;
#ifdef HAVE_LINUX
		/* Starting with the 0.99.13 Linux kernel all MSDOS
		   files have are "executables", so, when working with
		   msdos file systems, we have to ignore those bits ...
		   At least when displaying them in the panel. :-(  */
		this->dir_entry[entry].executable =
		    ((s.st_mode & 0111) && !this->msdosfs) ? 1 : 0;
#else   /* !HAVE_LINUX */
		this->dir_entry[entry].executable =
		    (s.st_mode & 0111) ? 1 : 0;
#endif  /* !HAVE_LINUX */
	    }
	    else
	    {
		this->dir_entry[entry].executable = 0;

		if (S_ISFIFO(s.st_mode))
		    this->dir_entry[entry].type = FIFO_ENTRY;
		else
		    if (S_ISSOCK(s.st_mode))
			this->dir_entry[entry].type = SOCKET_ENTRY;
		    else
			if (S_ISLNK(s.st_mode))
			{
			    struct stat s_tmp;
			    int stat_error =
				xstat(this->dir_entry[entry].name, &s_tmp);

			    if (stat_error == -1)
			    {
				/* This symbolic link has no target.  */
				this->dir_entry[entry].type = SYMLINK_ENTRY;
				sz = xreadlink(this->dir_entry[entry].name);
				s.st_size = (sz == -1) ? 0 : sz;
			    }
			    else
			    {
				/* The symbolic link has a target, and we
				   are going to display the target size,
				   not the link one.  Feel free to blame
				   me for this.  :-) */
				this->dir_entry[entry].type =
				    S_ISDIR(s_tmp.st_mode) ?  DIR_ENTRY :
							     FILE_ENTRY;
				s.st_size = s_tmp.st_size;
				/* Also take care if the link target is
				   an executable.  */
				this->dir_entry[entry].executable =
				    (s_tmp.st_mode & 0111) ? 1 : 0;
			    }
			}
			else
			    this->dir_entry[entry].type = FILE_ENTRY;
	    }
	}
	this->dir_entry[entry].size = s.st_size;
    }
    else
    {
	/* I think this is obsolete.  But I am keeping this it just in
	   case something wrong happens... :-) I think I've covered
	   symbolic links completely on the true branch of this `if'
	   statement.  However, if s.st_mode == 0, we might reach this
	   point.  */
	this->dir_entry[entry].type = SYMLINK_ENTRY;
	sz = xreadlink(this->dir_entry[entry].name);
	this->dir_entry[entry].size = (sz == -1) ? 0 : sz;
    }

    owner=getuser(s.st_uid);
    if(owner)
    {
	this->dir_entry[entry].owner=xmalloc(1 + max(strlen(owner),8));
	sprintf(this->dir_entry[entry].owner,"%-7s", owner);
    }
    else
    {
	this->dir_entry[entry].owner=xmalloc(32);
	sprintf(this->dir_entry[entry].owner,"%-7u",s.st_uid);
    }

    group=getgroup(s.st_gid);
    if(group)
    {
	this->dir_entry[entry].group=xmalloc(1 + max(strlen(group),8));
	sprintf(this->dir_entry[entry].group,"%-7s", group);
    }
    else
    {
	this->dir_entry[entry].group=xmalloc(32);
	sprintf(this->dir_entry[entry].group,"%-7u",s.st_gid);
    }

    this->dir_entry[entry].mtime = s.st_mtime;
    time = localtime(&s.st_mtime);

    if ((hour = time->tm_hour % 12) == 0)
	hour = 12;

    sprintf(this->dir_entry[entry].date,"%2d-%02d-%02d %2d:%02d%c",
	    time->tm_mon + 1, time->tm_mday, time->tm_year % 100,
	    hour, time->tm_min, (time->tm_hour < 12) ? 'a' : 'p');
}


int
panel_read_directory(this, directory, verify)
    panel_t *this;
    char *directory;
    int verify;
{
#ifdef HAVE_LINUX
    struct statfs fstat;
#endif  /* HAVE_LINUX */

    DIR *tmpdir;
    struct stat s;
    int namelen;
    char *old_path;
    struct dirent *d;
    int dotdot_found = 0;
    dir_entry_t *old_dir_entry = NULL, tmp;
    int entry, old_entries = 0, backdir_index = -1;


    tmpdir = opendir(directory);

    /* If the directory is on a NFS file system and we are superuser,
       opendir will not fail but readdir will report 0 files :-(.  */

    if (tmpdir == NULL)
    {
	if (strcmp(directory, "..") != 0)
	    return 0;

	/* Try harder.  This migh be due to the fact that we don't
	   have the ".." directory (NFS).  */

	directory = xgetcwd();

	if (directory == NULL)
	{
	    char *ptr;

	    /* FIXME: This isn't always deallocated!  We should use
	       alloca instead...  But is ugly anyway :-( */
	    directory = xstrdup(this->path);

	    ptr = strrchr(directory, '/');

	    if (ptr == NULL)
		fatal("bad directory");

	    *ptr = 0;
	}

	tmpdir = opendir(directory);

	if (tmpdir == NULL)
	{
	    xfree(directory);
	    return 0;
	}
    }

    if (chdir(directory) == -1)
    {
	closedir(tmpdir);
	return 0;
    }

    if (this->dir)
	closedir(this->dir);

    this->dir = tmpdir;
    old_path = xmalloc(this->pathlen + 1);
    strcpy(old_path, this->path);

    if (directory[0] == '/')
	this->path = xstrdup(directory);
    else
    {
	/* There is definitely a reason why this code is here, but I
	   don't remember it :-)  I think it might have been a problem
	   with entering a directory through a symbolic link which skips
	   a protected directory.  xgetcwd() will fail (due to the fact
	   that one directory in the real path is protected) and I had
	   to remove the last entry in the path to make it work.  */

	char *path = xgetcwd();

	if (path)
	{
	    xfree(this->path);
	    this->path = path;
	}
	else
	{
	    this->pathlen = strlen(this->path);

	    if (strcmp(directory, "..") == 0)
	    {
		char *ptr = strrchr(this->path, '/');

		if (ptr == NULL)
		    fatal("bad path");

		*ptr = 0;
	    }
	    else
	    {
		this->path = xrealloc(this->path, this->pathlen + 1 + 1 +
				      strlen(directory));
		strcat(this->path, "/");
		strcat(this->path, directory);
	     }
	}
    }

    minimize_path(this->path);
    this->pathlen = strlen(this->path);

    xstat(this->path, &s);

    if (s.st_size >= 2048)
    {
	il_message(PANEL_READ_DIR_MSG);
	tty_update();
    }

#ifdef HAVE_LINUX
    /* I can't get this number without including linux/msdos_fs.h :-(, so
       I've hard-coded it here.  */
    statfs(".", &fstat);
    this->msdosfs = fstat.f_type == 0x4d44;
#endif /* HAVE_LINUX */

    verify = verify &&
	     this->selected_entries &&
	     (strcmp(old_path, this->path) == 0);

    if (verify)
    {
	old_dir_entry = this->dir_entry;
	old_entries = this->entries;
	this->dir_entry = NULL;
    }
    else
	if (this->dir_entry)
	{
	    for (entry = 0; entry < this->entries; entry++)
		if (this->dir_entry[entry].name)
		{
		    xfree(this->dir_entry[entry].name);
		    this->dir_entry[entry].name = NULL;
		}

	    xfree(this->dir_entry);
	    this->dir_entry = NULL;
	}

    this->dir_entry = (dir_entry_t *)xmalloc(sizeof(dir_entry_t));
    this->selected_entries = 0;
    this->maxname = 0;

    for (this->entries = 0; (d = readdir(this->dir)); this->entries++)
    {
	/* Ignore the "." directory.  */
	if (d->d_name[0] == '.' && d->d_name[1] == 0)
	{
	    this->entries--;
	    continue;
	}

	if (strcmp(d->d_name, "..") == 0)
	{
	    dotdot_found = 1;

	    if (this->path[1])
		backdir_index = this->entries;
	    else
	    {
		/* Ignore ".." if this is the root directory.  */
		this->entries--;
		continue;
	    }
	}

	this->dir_entry = (dir_entry_t *)xrealloc(this->dir_entry,
						  (this->entries + 1) *
						  sizeof(dir_entry_t));
	if (verify)
	{
	    for (entry = 0; entry < old_entries; entry++)
		if (strcmp(d->d_name, old_dir_entry[entry].name) == 0)
		{
		    this->selected_entries +=
			(this->dir_entry[this->entries].selected =
			 old_dir_entry[entry].selected);
		    break;
		}

	    if (entry == old_entries)
		this->dir_entry[this->entries].selected = 0;
	}
	else
	    this->dir_entry[this->entries].selected = 0;

	/* Clear the fti cache flags.  */
	this->dir_entry[this->entries].fti_loaded = 0;

	namelen = strlen(d->d_name);
	this->dir_entry[this->entries].name = xmalloc(namelen + 1);
	strcpy(this->dir_entry[this->entries].name, d->d_name);
	this->maxname = max(this->maxname, namelen);

	panel_load_inode(this, this->entries);
    }

    /* Consistency check.  Some NFS file systems don't have the "."
       and ".." directory entries.  */

    /* We should have found the ".." directory.  */
    if (dotdot_found)
    {
	if (backdir_index != -1)
	    if (!S_ISDIR(this->dir_entry[backdir_index].mode))
		this->dir_entry[backdir_index].mode = S_IFDIR;
    }
    else
    {
	/* Build the ".." entry "by hand".  */
	this->dir_entry = (dir_entry_t *)xrealloc(this->dir_entry,
						  (this->entries + 1) *
						  sizeof(dir_entry_t));

	this->dir_entry[this->entries].selected = 0;

	/* Set the fti cache flags.  */
	this->dir_entry[this->entries].fti_loaded = 1;

	namelen = strlen("..");
	this->dir_entry[this->entries].name = xmalloc(namelen + 1);

	strcpy(this->dir_entry[this->entries].name, "..");
	this->maxname = max(this->maxname, namelen);

	panel_load_inode(this, this->entries);

	/* This is a directory.  */
	this->dir_entry[this->entries].mode = S_IFDIR;
	this->dir_entry[this->entries].type = DIR_ENTRY;

	this->entries++;
    }

    if (verify)
    {
	for (entry = 0; entry < old_entries; entry++)
	    if (old_dir_entry[entry].name)
	    {
		xfree(old_dir_entry[entry].name);
		old_dir_entry[entry].name = NULL;
	    }

	xfree(old_dir_entry);
    }

    CurrentSortMethod = this->sort_method;
    this->horizontal_offset = 0;

    if (backdir_index != -1)
    {
	tmp = this->dir_entry[0];
	this->dir_entry[0] = this->dir_entry[backdir_index];
	this->dir_entry[backdir_index] = tmp;

	qsort(this->dir_entry + 1, this->entries - 1,
	      sizeof(dir_entry_t), sort_compare_fn);
    }
    else
	qsort(this->dir_entry, this->entries,
	      sizeof(dir_entry_t), sort_compare_fn);

    panel_set_current_entry(this, min(this->current_entry, this->entries - 1));

    xfree(old_path);
    return 1;
}


void
panel_init_iterator(this)
    panel_t *this;
{
    this->last_index = -1;
    this->multiple_files = this->selected_entries;
}


int
panel_get_next(this)
    panel_t *this;
{
    int i;

    if (this->multiple_files)
    {
	for (i = this->last_index + 1; i < this->entries; i++)
	    if (this->dir_entry[i].selected)
		return this->last_index = i;

	return -1;
    }
    else
    {
	if (this->last_index == 0)
	    return -1;

	this->last_index = 0;

	/* In the root directory there is no '..' entry, so the first
	   entry can be returned.  */
	if (rootdir())
	    return this->current_entry;
	else
	    return (this->current_entry != 0) ? this->current_entry : -1;
    }
}


void
panel_update_entries(this)
    panel_t *this;
{
    int i, limit;
    tty_status_t status;

    assert(this->current_entry < this->entries);

    if (!this->visible)
	return;

    tty_save(&status);

    for (i = this->first_on_screen;
	 (i < this->entries) && (i - this->first_on_screen < this->lines - 2);
	 i++)
	panel_update_entry(this, i);

    tty_colors(OFF, WHITE, PanelBackground);

    memset(this->temp, ' ', this->columns);
    limit = min(this->lines - 2, this->on_screen);

    for (; i < limit; i++)
    {
	window_goto(this->window, i - this->first_on_screen + 1, 1);
	window_puts(this->window, this->temp, this->columns - 2);
    }

    this->on_screen = this->entries;
    tty_restore(&status);
}


void
panel_update_path(this)
    panel_t *this;
{
    size_t len;
    tty_status_t status;

    if (this->columns < (1 + 1 + 1 + 1 + 14 + 1 + 1))
	return;

    if (!this->visible)
	return;

    tty_save(&status);

    len = this->columns - (1 + 1 + 1 + 14 + 1 + 1);
    memset(this->temp, ' ', this->columns);
    truncate_long_name(this->path, this->temp, len);
    toprintable(this->temp, len);

    tty_colors(PanelPathBrightness, PanelPath, PanelFrame);

    window_goto(this->window, 0, 2);
    window_puts(this->window, this->temp, len + 1);

    tty_restore(&status);
}

static char *
panel_beautify_number(buf, number, inflags)
    char *buf;
    off64_t number;
    int inflags;
{
    int flags = ( inflags | human_ceiling | human_suppress_point_zero );
    return human_readable(number, buf, flags, 1, 1);
}

static int panel_fit_number(buf, number, flags, maxlen)
    char *buf;
    off64_t number;
    int flags;
    int maxlen;
{
    char *str=NULL;
    int scaled=(flags & human_autoscale);
    str=panel_beautify_number(buf,number,(flags | (GroupDigits ? human_group_digits : 0)));
    /* if too big, force autoscale */
    if((str == NULL) || (strlen(str) > maxlen))
    {
	scaled=1;
	str=panel_beautify_number(buf, number, (flags|human_autoscale|human_SI|human_base_1024));
    }
    strncpy(buf,str,maxlen);
    /* just in case */
    buf[maxlen]='\0';
    return scaled;
}


static char *
panel_beautify_info_number(buf, number, width, numfiles)
    char *buf;
    off64_t number;
    int width;
    int numfiles;
{
    char shortnumbuf[LONGEST_HUMAN_READABLE+1];
    char *shortnum;
    char longnumbuf[LONGEST_HUMAN_READABLE+1];
    char filesbuf[LONGEST_HUMAN_READABLE+1];
    char *filestr;
    int scaled;

    filestr=panel_beautify_number(filesbuf, (off64_t) numfiles,
				  (GroupDigits ? human_group_digits : 0));

/* " (xxxxxPB) in yyyyy files", not including yyyyy */
#define STATIC_TEXT_LEN 18
    scaled=panel_fit_number(longnumbuf, number, 0,
			    (width - (STATIC_TEXT_LEN + strlen(filestr))));
    if ( (!scaled) && (number > 999) )
    {
	shortnum=panel_beautify_number(shortnumbuf, number,
				       (human_SI|human_autoscale|human_base_1024));
	sprintf(buf, "%s (%sB) in %s file%s", longnumbuf, shortnum,
		filestr, (numfiles > 1) ? "s" : "");
    }
    else
    {
	sprintf(buf, "%s in %s file%s", longnumbuf, filestr, (numfiles > 1) ? "s" : "");
    }
    return buf;
}



void
panel_update_size(this)
    panel_t *this;
{
    char buf[LONGEST_HUMAN_READABLE+3];
    char *sz;
    tty_status_t status;
    struct fs_usage fsu;
    int viewable = this->columns - (1 + 1 + 1 + 1);

    if (!this->visible)
	return;

    tty_save(&status);

    fsu.fsu_blocks = (uintmax_t) -1;

    /* get_fs_usage will fail on SVR2 (needs disk instead of NULL) */
    if (viewable < 6 ||
	get_fs_usage(this->path, NULL, &fsu) < 0 ||
	fsu.fsu_blocks == (uintmax_t) -1)
    {
	memset(buf, ' ', sizeof(buf));
	sz=buf;
	tty_brightness(OFF);
	tty_foreground(PanelFrame);
    }
    else
    {
	off64_t n;
	off64_t free_blocks = fsu.fsu_bavail;

	/* Indeed, this might happen...  */
	if (free_blocks < 0)
	    free_blocks = 0;

	n = free_blocks * fsu.fsu_blocksize;

	sz=panel_beautify_number(buf, n, (human_autoscale|human_SI|human_base_1024));
	sz=strcat(sz,"B");
	tty_brightness(PanelDeviceFreeSpaceBrightness);
	tty_foreground(PanelDeviceFreeSpace);
    }

    tty_background(PanelFrame);

    window_goto(this->window, 0, this->columns - 2 - min(strlen(sz), viewable));
    window_puts(this->window, sz, min(strlen(sz), viewable));

    tty_restore(&status);
}


static void
panel_mode2string(this, entry, string)
    panel_t *this;
    int entry;
    char *string;
{
    int i;
    mode_t mode;

    strcpy(string, rights);
    mode = this->dir_entry[entry].mode;

#ifdef S_ISREG
    if (S_ISREG(mode))
	string[0] = '-';
    else
#endif /* S_ISREG */
#ifdef S_ISDIR
	if (S_ISDIR(mode))
	    string[0] = 'd';
	else
#endif /* S_ISDIR */
#ifdef S_ISCHR
	    if (S_ISCHR(mode))
		string[0] = 'c';
	    else
#endif /* S_ISCHR */
#ifdef S_ISBLK
		if (S_ISBLK(mode))
		    string[0] = 'b';
		else
#endif /* S_ISBLK */
#ifdef S_ISFIFO
		    if (S_ISFIFO(mode))
			string[0] = 'p';
		    else
#endif /* S_ISFIFO */
#ifdef S_ISSOCK
			if (S_ISSOCK(mode))
			    string[0] = 's';
			else
#endif /* S_ISSOCK */
#ifdef S_ISLNK
			    if (S_ISLNK(mode))
				string[0] = 'l';
			    else
#endif /* S_ISLNK */
				string[0] = '?';

    for (i = 0; i < 9; mode >>= 1, i++)
	if ((mode & 1) == 0)
	    string[9 - i] = '-';

    mode = this->dir_entry[entry].mode;

#ifdef S_ISUID
    if (mode & S_ISUID)
	string[3] = (string[3] == 'x') ? 's' : 'S';
#endif /* S_ISUID */

#ifdef S_ISGID
    if (mode & S_ISGID)
	string[6] = (string[6] == 'x') ? 's' : 'S';
#endif /* S_ISGID */

#ifdef S_ISVTX
    if (mode & S_ISVTX)
	string[9] = (string[9] == 'x') ? 't' : 'T';
#endif /* S_ISVTX */
}


void
panel_update_info(this)
    panel_t *this;
{
    tty_status_t status;
    size_t len, maxname;
    off64_t total_size = 0;
    char str[1024];
    char temp_rights[16];

    assert(this->current_entry < this->entries);

    if (!this->visible)
	return;

    tty_save(&status);

    if (this->lines >= 3)
    {
	/* This is an ugly hack, I know, but it does what I want.  */
	tty_colors(OFF, PanelFrame, PanelFrame);
	window_goto(this->window, this->thumb + 1, this->columns - 1);
	window_putc(this->window, ' ');

	if (this->entries > this->lines-2)
	{
	    this->thumb = ((this->current_entry * (this->lines-2)) /
			   this->entries);

	    tty_colors(PanelThumbBrightness,
		       PanelThumbForeground,
		       PanelThumbBackground);
	    window_goto(this->window, this->thumb+1, this->columns-1);
	    window_putc(this->window, '#');
	}
    }

    if (this->columns < 40)
    {
	if (this->columns < (1 + 1 + 10 + 1 + 1))
	    *str = '\0';
	else
	{
	    panel_mode2string(this, this->current_entry, temp_rights);
	    sprintf(str, "%10s", temp_rights);
	}

	goto skip_info_display;
    }

    if (this->selected_entries)
    {
	int entry;
	for (entry = 0; entry < this->entries; entry++)
	    if  (this->dir_entry[entry].selected &&
		 this->dir_entry[entry].type == FILE_ENTRY)
		total_size += this->dir_entry[entry].size;
	panel_beautify_info_number(str, total_size, this->columns, this->selected_entries);
	tty_brightness(PanelFilesInfoBrightness);
	tty_foreground(PanelFilesInfo);
    }
    else
    {
	if (InfoDisplay == OFF)
	{
	    *str = '\0';
	    goto skip_info_display;
	}

	panel_mode2string(this, this->current_entry, temp_rights);
	maxname = this->columns - 26;  /* FIXME: Huh?  */
	len = min(strlen(this->dir_entry[this->current_entry].name), maxname);
	memcpy(str, this->dir_entry[this->current_entry].name, len);
	memset(str + len, ' ', maxname - len);

	if (this->dir_entry[this->current_entry].type == DIR_ENTRY)
	    sprintf(str + maxname, " %10s %10s",
		    (strcmp(this->dir_entry[this->current_entry].name, "..") ==
		     0) ?
		    " UP-DIR" : "SUB-DIR", temp_rights);
	else
	{
	    char size[LONGEST_HUMAN_READABLE+1];
	    panel_fit_number(size,this->dir_entry[this->current_entry].size,0,10);
	    sprintf(str + maxname, " %10s %10s",size,temp_rights);
	}

      skip_info_display:
	tty_brightness(PanelFileInfoBrightness);
	tty_foreground(PanelFileInfo);
    }

    memcpy(this->temp, str, len = strlen(str));
    if( (len+2) < this->columns)
	memset(this->temp + len, ' ', this->columns - 2 - len);
    toprintable(this->temp, len);
    tty_background(PanelFrame);
    window_goto(this->window, this->lines - 1, 2);
    window_puts(this->window, this->temp, this->columns - 4);

    tty_restore(&status);
}


void
panel_build_entry_field(this, entry, display_mode, offset)
    panel_t *this;
    int entry, display_mode, offset;
{
    char temp_rights[16];
    char hbuf[LONGEST_HUMAN_READABLE+1];

    switch (display_mode)
    {
	case ENABLE_OWNER_GROUP:
	    memcpy(this->temp + this->columns - 2 - offset,
		   this->dir_entry[entry].owner, 7);
	    memcpy(this->temp + this->columns - 2 - offset + 8,
		   this->dir_entry[entry].group, 7);
	    break;

	case ENABLE_DATE_TIME:
	    memcpy(this->temp + this->columns - 2 - offset,
		   this->dir_entry[entry].date, 15);
	    break;

	case ENABLE_SIZE:
	case ENABLE_ABBREVSIZE:
	{
	    char *ptr;
	    int buflen;
	    int flags = 0;
	    if((display_mode==ENABLE_ABBREVSIZE) ||
	       (strlen(human_readable(this->dir_entry[entry].size,
				      hbuf, human_ceiling, 1, 1)) > MaxUnscaledDigits))
	    {
		flags |= (human_autoscale | human_SI |human_base_1024);
	    }
	    panel_fit_number(hbuf, this->dir_entry[entry].size, flags, 10);
	    buflen=min(strlen(hbuf),10);
	    ptr=this->temp + this->columns - 2 - offset;
	    if(buflen < 10)
	    {
		memset(ptr,' ',10-buflen);
		ptr += (10-buflen);
	    }
	    memcpy(ptr, hbuf, buflen);
	    break;
	}

	case ENABLE_MODE:
	    panel_mode2string(this, entry, temp_rights);
	    memcpy(this->temp + this->columns - 2 - offset, temp_rights, 10);
	    break;

	case ENABLE_FULL_NAME:
	    /* This should not be called...  */
	    assert(0);
#if 0
	    /* file name     -> 20 characters (at least)
	       owner + group -> 16 characters
	       date  + time  -> 16 characters
	       size          -> 11 characters
	       mode          -> 11 characters */
	    if (this->columns < 20 + 16 + 16 + 11 + 11)
		break;
#endif
	    break;

	case ENABLE_ALL:
	    break;

	default:
	    fatal("invalid mode");
    }
}


static int reserved_characters[FILE_DISPLAY_MODES] =
{
    1 + 1 +                16 + 1 + 1,
    1 + 1 +                16 + 1 + 1,
    1 + 1 +                11 + 1 + 1,
    1 + 1 +                11 + 1 + 1,
    1 + 1 +                11 + 1 + 1,
    1 + 1 +                 0 + 1 + 1,
    1 + 1 + 16 + 16 + 11 + 11 + 1 + 1,
};


static int
panel_get_reserved_characters(this)
    panel_t *this;
{
    if ((this->columns < 40) ||
	((this->columns < 20 + 16 + 16 + 11 + 11) &&
	 (this->display_mode == ENABLE_ALL)))
	return 1 + 1 + 1 + 1;
    else
	return reserved_characters[this->display_mode];
}


void
panel_update_entry(this, entry)
    panel_t *this;
    int entry;
{
    char c = '\0';
    int foreground, background, brightness;
    size_t len, reserved, entry_length, offset;

    assert(this->current_entry < this->entries);

    if (!this->visible)
	return;

    memset(this->temp, ' ', this->columns);
    reserved = panel_get_reserved_characters(this);

    if ((entry > 0) || rootdir())
	switch (this->dir_entry[entry].type)
	{
	    case FILE_ENTRY:
		if (this->dir_entry[entry].executable)
		    c = '*';
		break;

	    case DIR_ENTRY:     c = '/'; break;
	    case SYMLINK_ENTRY: c = '@'; break;
	    case FIFO_ENTRY:    c = '|'; break;
	    case SOCKET_ENTRY:  c = '='; break;
	}

    if (c != '\0')
	reserved++;

    entry_length = strlen(this->dir_entry[entry].name);

    if (this->columns - reserved >= entry_length)
	offset = 0;
    else
	if (this->columns - reserved + this->horizontal_offset > entry_length)
	    offset = entry_length - (this->columns - reserved);
	else
	    offset = this->horizontal_offset;

    len = min(entry_length - offset, this->columns - reserved);
    memcpy(&this->temp[1], this->dir_entry[entry].name + offset, len);

    toprintable(&this->temp[1], len);

    if (c != '\0')
	this->temp[len + 1] = c;

    if (this->columns >= 40)
	switch (this->display_mode)
	{
	    case ENABLE_OWNER_GROUP:
	    case ENABLE_DATE_TIME:
		panel_build_entry_field(this, entry, this->display_mode, 16);
		break;

	    case ENABLE_SIZE:
	    case ENABLE_ABBREVSIZE:
	    case ENABLE_MODE:
		panel_build_entry_field(this, entry, this->display_mode, 11);
		break;

	    case ENABLE_FULL_NAME:
		/* Don't call panel_build_entry_field(), it's useless.  */
		break;

	    case ENABLE_ALL:
		/* file name     -> 20 characters (at least)
		   owner + group -> 16 characters
		   date  + time  -> 16 characters
		   size          -> 11 characters
		   mode          -> 11 characters */

		if (this->columns < 20 + 16 + 16 + 11 + 11)
		    break;

		panel_build_entry_field(this, entry, ENABLE_OWNER_GROUP,
					16 + 16 + 11 + 11);
		panel_build_entry_field(this, entry, ENABLE_DATE_TIME,
					16 + 11 + 11);
		panel_build_entry_field(this, entry, ENABLE_SIZE,
					11 + 11);
		panel_build_entry_field(this, entry, ENABLE_MODE,
					11);
		break;

	    default:
		fatal("invalid mode");
	}

    if (this->dir_entry[entry].selected)
	this->temp[this->columns - 3] = '*';

    if (entry == this->current_entry)
	this->temp[0] = this->focus ?
	    ACTIVE_PANEL_MARKER : INACTIVE_PANEL_MARKER;

    if (TypeSensitivity && this->dir_entry[entry].type != DIR_ENTRY &&
	!this->dir_entry[entry].selected)
    {
	/* Notify about the cursor movement just once.  */
	window_goto(this->window, entry - this->first_on_screen + 1, 1);

	/* Display the first character of the entry.  That character is
	   either a space or the '>' character (the current file
	   marker).  */
	brightness = this->dir_entry[entry].selected ?
			 PanelSelectedFileBrightness :
			 PanelNotSelectedFileBrightness;
	foreground = this->dir_entry[entry].selected ?
			 PanelSelectedFile :
			 PanelNotSelectedFile;

	if (entry == this->current_entry && this->focus == ON)
	    background = PanelCurrentFile;
	else
	    background = PanelBackground;

	tty_colors(brightness, foreground, background);

	window_putc(this->window, *this->temp);

	/* Try to match the current file name against the specified
	   patterns.  Display it with the appropriate color.  */
	if (this->dir_entry[entry].fti_loaded == 0)
	    panel_load_fti(this, entry);

	if (this->dir_entry[entry].brightness != 2)
	{
	    if (entry == this->current_entry && this->focus == ON)
		tty_colors(brightness,
			   this->dir_entry[entry].foreground,
			   background);
	    else
		tty_colors(this->dir_entry[entry].brightness,
			   this->dir_entry[entry].foreground,
			   this->dir_entry[entry].background);
	}

	window_puts(this->window, this->temp + 1, len + 1);

	/* Display the end of the entry (the part after the file name).  */
	window_puts(this->window,
		    this->temp + 1 + len + 1,
		    this->columns - len - 2 - 2);

	tty_colors(brightness, foreground, background);
    }
    else
    {
	if (entry == this->current_entry && this->focus == ON)
	{
	    foreground = this->dir_entry[entry].selected ?
			     PanelCurrentSelectedFile :
			     PanelCurrentNotSelectedFile;
	    background = PanelCurrentFile;
	}
	else
	{
	    foreground = this->dir_entry[entry].selected ?
			     PanelSelectedFile :
			     PanelNotSelectedFile;
	    background = PanelBackground;
	}

	brightness = this->dir_entry[entry].selected ?
			 PanelSelectedFileBrightness :
			 PanelNotSelectedFileBrightness;

	tty_colors(brightness, foreground, background);

	window_goto(this->window, entry - this->first_on_screen + 1, 1);
	window_puts(this->window, this->temp, this->columns - 2);
    }
}


void
panel_update_frame(this)
    panel_t *this;
{
    int line;
    tty_status_t status;
    char *buf = xmalloc(this->columns);

    if (!this->visible)
	return;

    tty_save(&status);

    tty_colors(OFF, PanelFrame, PanelFrame);

    for (line = 1; line < this->lines - 1; line++)
    {
	window_goto(this->window, line, 0);
	window_putc(this->window, ' ');
    }

    for (line = 1; line < this->lines - 1; line++)
    {
	window_goto(this->window, line, this->columns - 1);
	window_putc(this->window, ' ');
    }

    memset(buf, ' ', this->columns);
    window_goto(this->window, 0, 0);
    window_puts(this->window, buf, this->columns);
    window_goto(this->window, this->lines - 1, 0);
    window_puts(this->window, buf, this->columns);
    xfree(buf);

    tty_restore(&status);
}


void
panel_update(this)
    panel_t *this;
{
    assert(this->current_entry < this->entries);

    panel_update_frame(this);
    panel_update_path(this);
    panel_update_info(this);
    panel_update_size(this);
    panel_update_entries(this);
}


void
panel_set_focus(this, status)
    panel_t *this;
    int status;
{
    this->focus = status;
    panel_update_entry(this, this->current_entry);

    if (this->focus)
	if (chdir(this->path) == -1)
	{
	    panel_recover(this);
	    panel_update(this);
	}
}


void
panel_select_all(this)
    panel_t *this;
{
    int entry;

    for (entry = 0; entry < this->entries; entry++)
	if (this->dir_entry[entry].selected == 0 &&
	    this->dir_entry[entry].type != DIR_ENTRY)
	{
	    this->dir_entry[entry].selected = 1;
	    this->selected_entries++;
	}
}


void
panel_unselect_all(this)
    panel_t *this;
{
    int entry;

    for (entry = 0; entry < this->entries; entry++)
	this->dir_entry[entry].selected = 0;

    this->selected_entries = 0;
}


char *
panel_get_path(this)
    panel_t *this;
{
    return this->path;
}


int
canceled()
{
    int key;

    if (user_heart_attack)
    {
	input_line_t *saved_il;

	user_heart_attack = 0;
	saved_il = il_save();
	key = panel_1s_message("Abort current operation? ", "yn",
			       IL_FREEZED | IL_BEEP);
	il_restore(saved_il);
	il_update();
	il_update_point();
	tty_update();
	return (key == 'n' || key == 'N') ? 0 : 1;
    }

    return 0;
}


/*
 * Check if two file names point to the same file.  It works by
 * checking the devices and inodes.
 */
int
same_file(file1, file2)
    char *file1;
    char *file2;
{
    struct stat s1;
    struct stat s2;

    if (xstat(file1, &s1) == 0 &&
	xstat(file2, &s2) == 0 &&
	s1.st_dev == s2.st_dev &&
	s1.st_ino == s2.st_ino)
	return 1;

    return 0;
}


#define WARN_OVERWRITE	0
#define WARN_OK		1
#define WARN_CANCEL	2
#define WARN_SKIP	3


int
panel_warning(this, file)
    panel_t *this;
    char *file;
{
    char c;

    if (this->selected_entries)
	c = panel_2s_message("%s: File exists. Overwrite/Skip/All/Cancel? ",
			     file, "osac", IL_MOVE|IL_BEEP|IL_SAVE|IL_ERROR);
    else
	c = panel_2s_message("%s: File exists. Overwrite/Cancel? ",
			     file, "oc", IL_MOVE|IL_BEEP|IL_SAVE|IL_ERROR);

    switch (c)
    {
	case 'o':
	    return WARN_OVERWRITE;

	case 'a':
	    if (this->selected_entries)
	    {
		this->chkdest = OFF;
		return WARN_OVERWRITE;
	    }

	    break;

	case 's':
	    if (this->selected_entries)
		return WARN_SKIP;

	default:
	    break;
    }

    return WARN_CANCEL;
}


/*
 * Remove a directory entry only if it is not a special file.
 */
void
panel_unlink(name)
    char *name;
{
    struct stat statbuf;

    /* Don't delete the file unless it is a regular one.  Also don't
       delete it if we cannot stat it.  Avoid doing anything stupid.  */
    if (xstat(name, &statbuf) == 0 && S_ISREG(statbuf.st_mode))
	unlink(name);
}


#define COPY_BUFFER_SIZE	(32 * 1024)

#define SD_OK		WARN_OK
#define SD_CANCEL	WARN_CANCEL
#define SD_SKIP		WARN_SKIP
#define S_OPENERR	 4
#define S_READERR	 5
#define D_CREATERR	 6
#define D_WRITEERR	 7
#define SD_NOSPACE	 8
#define SD_UNKNOWN	 9
#define D_STATERR	10
#define SD_INVAL	11
#define SD_INTERRUPTED  12

char *copyerr[12] =
{
    "",
    "",
    "",
    "cannot open source file",
    "cannot read from source file",
    "cannot create destination file",
    "cannot write to destination file",
    "not enough space on device",
    "unknown error",
    "cannot stat destination file",
    "cannot copy a directory to a non-directory",
    "cp was interrupted by a signal",
};


static int
panel_percent(x, total)
    off64_t x;
    off64_t total;
{
    /* This shouldn't happen.  */
    if (total == 0)
	return 0;

    return (total >= 100000) ? (x / (total / 100)) : ((x * 100) / total);
}


int
panel_copy(this, src, dest, mode, uid, gid)
    panel_t *this;
    char *src;
    char *dest;
    mode_t mode;
    uid_t uid;
    gid_t gid;
{
    size_t len;
    int sfd, dfd, error;
    off64_t flen, n, memsize;
    struct stat dest_statbuf;
    char *buf, *dest_file, *msg;
    int bytes_transferred, bytes_to_transfer;

    if (S_ISLNK(mode))
    {
	struct stat s_tmp;
	if(xstat(src,&s_tmp) != -1)
	{
	    mode=s_tmp.st_mode;
	}
    }

    if (S_ISDIR(mode))
    {
	/* The source file is a directory.  */
	int result;
	char *temp;

	if (xstat(dest, &dest_statbuf) == 0)
	{
	    /* The destination exists.  */

	    /* We can't copy a directory to a non-directory.  */
	    if (!S_ISDIR(dest_statbuf.st_mode))
		return SD_INVAL;

	    /* The destination is a directory.  The directory 'dest'
	       might contain a directory 'src' so we have to check
	       it out.  */
	    temp = xmalloc(strlen(dest) + 1 + strlen(src) + 1);

	    sprintf(temp, "%s/%s", dest, src);

	    if (this->chkdest && access(temp, 0) == 0)
	    {
		status_default();
		error = panel_warning(this, temp);
		tty_update();
		xfree(temp);

		if (error)
		    return error;
	    }
	    else
		xfree(temp);
	}

	/* The 'dest' directory contains no 'src' directory or we have
	   the permission to overwrite it, so we may proceed.  */
	temp = xmalloc(32 + strlen(src) + strlen(dest) + 1);

	dest_file = xbasename(dest);

	if (*dest_file == '\0')
	    return D_CREATERR;

	msg = xmalloc(32 + strlen(src) + strlen(dest));
	sprintf(msg, "(COPY) cp -r \"%s\" \"%s\"", src, dest);
	status(msg, STATUS_WARNING, STATUS_LEFT);
	tty_update();
	xfree(msg);

	sprintf(temp, "cp -r \"%s\" \"%s\"", src, dest);
	result = start(temp, 1);
	xfree(temp);

	tty_update_title(this->path);

	if (WIFSIGNALED(result))
	{
	    return SD_INTERRUPTED;
	}

	if(WIFEXITED(result) && (WEXITSTATUS(result) != 0))
	{
	    display_errors("cp");
	    return SD_UNKNOWN;
	}
	return SD_OK;
    }

    /* The source is a regular file.  */
    len = strlen(dest);
    dest = xstrdup(dest);

    if (xstat(dest, &dest_statbuf) == 0 && S_ISDIR(dest_statbuf.st_mode))
    {
	/* The destination is a directory.  */
	dest = xrealloc(dest, len + 1 + strlen(src) + 1);
	strcat(dest, "/");
	strcat(dest, src);
    }

    dest_file = xbasename(dest);

    if (*dest_file == '\0')
    {
	xfree(dest);
	return D_CREATERR;
    }

    if (this->chkdest && (access(dest, 0) == 0))
    {
	status_default();
	error = panel_warning(this, dest_file);
	tty_update();

	if (error)
	    return error;
    }

    msg = xmalloc(32 + strlen(src));

    if (S_ISREG(mode))
	sprintf(msg, "(COPY) [  0%%] %s", src);
    else
	sprintf(msg, "(COPY) [0 bytes] %s", src);

    status(msg, STATUS_WARNING, STATUS_LEFT);
    tty_update();
    xfree(msg);

    if ((sfd = open64(src, O_RDONLY | O_BINARY)) == -1)
	return S_OPENERR;

#ifdef HAVE_LINUX
    /* Ignore the executable bits when copying from a MSDOG file system.  */
    if (this->msdosfs)
	mode &= ~0111;
#endif  /* HAVE_LINUX */

    if ((dfd = creat(dest, mode)) == -1)
    {
	close(sfd);
	xfree(dest);
	return D_CREATERR;
    }

    memsize = min(flen = get_file_length(sfd), COPY_BUFFER_SIZE);

    if (S_ISBLK(mode) || S_ISCHR(mode))
    {
	flen = MAXFILESIZE;
	memsize = COPY_BUFFER_SIZE;
    }

    if (flen == 0)
    {
	if (getuid() == 0)
	    chown(dest, uid, gid);
	close2(sfd, dfd);
	xfree(dest);
	return SD_OK;
    }

    buf = xmalloc(memsize);

    for (n = 0; n < flen; n += COPY_BUFFER_SIZE)
    {
	bytes_to_transfer = min(flen - n, memsize);

	if (canceled())
	{
	    close2(sfd, dfd);
	    panel_unlink(dest);
	    xfree2(buf, dest);
	    return SD_CANCEL;
	}

	signals(ON);
	bytes_transferred = xread(sfd, buf, bytes_to_transfer);
	signals(OFF);

	if (bytes_transferred != bytes_to_transfer)
	{
	    if (bytes_transferred >= 0)
	    {
		if (bytes_transferred)
		    bytes_to_transfer = bytes_transferred;
		else
		{
		    if (getuid() == 0)
			chown(dest, uid, gid);
		    close2(sfd, dfd);
		    xfree2(buf, dest);
		    return SD_OK;
		}
	    }
	    else
	    {
		close2(sfd, dfd);
		panel_unlink(dest);
		xfree2(buf, dest);
		return S_READERR;
	    }
	}

	if (canceled())
	{
	    close2(sfd, dfd);
	    panel_unlink(dest);
	    xfree2(buf, dest);
	    return SD_CANCEL;
	}

	bytes_transferred = xwrite(dfd, buf, bytes_to_transfer);

	if (bytes_transferred != bytes_to_transfer)
	{
	    int safe_errno = errno;

	    close2(sfd, dfd);
	    panel_unlink(dest);
	    xfree2(buf, dest);
	    return (bytes_transferred > 0) ? SD_NOSPACE :
		((safe_errno == ENOSPC) ? SD_NOSPACE : D_WRITEERR);
	}

	if (n + bytes_to_transfer <= flen)
	{
	    msg = xmalloc(32 + strlen(src));

	    if (S_ISREG(mode))
		sprintf(msg, "(COPY) [%3d%%] %s",
			panel_percent(n + bytes_to_transfer, flen),
			src);
	    else
	    {
#ifdef HAVE_64BIT_IO
		sprintf(msg, "(COPY) [%Ld bytes] %s",
			(long long)(n + bytes_to_transfer), src);
#else /* !HAVE_64BIT_IO */
		sprintf(msg, "(COPY) [%ld bytes] %s",
			(long)(n + bytes_to_transfer), src);
#endif /* !HAVE_64BIT_IO */
	    }
	    status(msg, STATUS_WARNING, STATUS_LEFT);
	    tty_update();
	    xfree(msg);
	}
    }

    if (getuid() == 0)
	chown(dest, uid, gid);

    close2(sfd, dfd);
    xfree2(buf, dest);

    return SD_OK;
}


#define FT_OK		WARN_OK
#define FT_CANCEL	WARN_CANCEL
#define FT_SKIP		WARN_SKIP
#define T_CREATERR	 4
#define F_DELETERR	 5
#define F_STATERR	 6
#define T_STATERR	 7
#define FT_UNKNOWN	 8
#define FT_INVAL	 9
#define FT_NOSPACE	10
#define FT_COPY		11
#define FT_INTERRUPTED  12


char *moveerr[12] =
{
    "",
    "",
    "",
    "cannot create destination file",
    "cannot remove source file",
    "cannot stat source file",
    "cannot stat destination directory",
    "unknown error",
    "cannot copy a directory to a non-directory",
    "not enough space on device",
    "cannot copy file",
    "mv was interrupted by a signal",
};


int
panel_move(this, from, to, mode)
    panel_t *this;
    char *from, *to;
    mode_t mode;
{
    int error;
    size_t len;
    struct stat to_statbuf;
    struct stat from_statbuf;
    char *to_file, *msg;


    if (S_ISDIR(mode))
    {
	/* The source file is a directory.  */
	int result;
	char *temp;

	if (xstat(to, &to_statbuf) == 0)
	{
	    /* The destination exists.  */

	    /* We can't move a directory to a non-directory.  */
	    if (!S_ISDIR(to_statbuf.st_mode))
		return FT_INVAL;

	    /* The destination is a directory.  The directory 'to'
	       might contain a directory 'from' so we have to check
	       it out.  */
	    temp = xmalloc(strlen(to) + 1 + strlen(from) + 1);

	    sprintf(temp, "%s/%s", to, from);

	    if (this->chkdest && access(temp, 0) == 0)
	    {
		status_default();
		error = panel_warning(this, temp);
		tty_update();
		xfree(temp);

		if (error)
		    return error;
	    }
	    else
		xfree(temp);
	}

	/* The 'to' directory contains no 'from' directory or we have
	   the permission to overwrite it, so we may proceed.  */
	temp = xmalloc(32 + strlen(from) + strlen(to) + 1);

	to_file = xbasename(to);

	if (*to_file == '\0')
	    return T_CREATERR;

	msg = xmalloc(32 + strlen(from) + strlen(to));
	sprintf(msg, "(MOVE) mv -f \"%s\" \"%s\"", from, to);
	status(msg, STATUS_WARNING, STATUS_LEFT);
	tty_update();
	xfree(msg);

	sprintf(temp, "mv -f \"%s\" \"%s\"", from, to);
	result = start(temp, 1);
	xfree(temp);

	tty_update_title(this->path);

	if (WIFSIGNALED(result))
	    return FT_INTERRUPTED;

	if(WIFEXITED(result))
	{
	    if(WEXITSTATUS(result) != 0)
	    {
		display_errors("mv");
		return FT_UNKNOWN;
	    }
	    return FT_OK;
	}
	return FT_UNKNOWN;
    }

    /* The source is not a directory.  */
    len = strlen(to);
    to  = xstrdup(to);

    if (xstat(to, &to_statbuf) == 0 && S_ISDIR(to_statbuf.st_mode))
    {
	/* The destination is a directory.  */
	to = xrealloc(to, len + 1 + strlen(from) + 1);
	strcat(to, "/");
	strcat(to, from);
    }

    to_file = xbasename(to);

    if (*to_file == '\0')
    {
	xfree(to);
	return T_CREATERR;
    }

    if (to_file == to)
    {
	/* 'to' is relative to the current directory.  We have to add './'
	   to its beginning in order to be able to stat the destination
	   directory later.  */
	char *temp = xmalloc(2 + (len = strlen(to)) + 1);

	temp[0] = '.';
	temp[1] = '/';
	memcpy(temp + 2, to, len + 1);
	xfree(to);
	to = temp;

	/* Update 'to_file'.  */
	to_file = to + 2;
    }

    if (this->chkdest && (access(to, 0) == 0))
    {
	status_default();
	error = panel_warning(this, to_file);
	tty_update();

	if (error)
	    return error;
    }

    msg = xmalloc(32 + strlen(from));
    sprintf(msg, "(MOVE) %s", from);
    status(msg, STATUS_WARNING, STATUS_LEFT);
    tty_update();
    xfree(msg);

    if (xstat(from, &from_statbuf) == -1)
	return F_STATERR;

    if (xstat(to, &to_statbuf) == -1)
    {
	/* This is very very ugly ... :-(.  We need to stat the destination
	    directory in order to find out if we can move the file.  If we
	    can't, we have to copy it.  */
	char c = *(to_file - 1);

	*(to_file - 1) = 0;
	error = (*to) ? xstat(to, &to_statbuf) : xstat("/", &to_statbuf);
	*(to_file - 1) = c;

	if (error == -1)
	    return T_STATERR;
    }

    if (to_statbuf.st_dev != from_statbuf.st_dev
#ifdef HAVE_LINUX
	|| this->msdosfs
#endif /* HAVE_LINUX */
	)
    {
	error = panel_copy(this, from, to,
			   from_statbuf.st_mode,
			   from_statbuf.st_uid,
			   from_statbuf.st_gid);

	switch (error)
	{
	    case SD_OK:		goto remove_from;
	    case SD_CANCEL:	return FT_CANCEL;
	    case SD_SKIP:	return FT_SKIP;
	    case SD_NOSPACE:	return FT_NOSPACE;

	    default:
		panel_3s_message("%s: Copy failed, %s.",
				 from, copyerr[error - 1],
				 (char *)NULL,
				 IL_MOVE | IL_BEEP | IL_ERROR);
		return FT_COPY;
	}

	/* Not reached.  */
    }

    service_pending_signals();

    if (S_ISREG(to_statbuf.st_mode) || S_ISDIR(to_statbuf.st_mode))
    {
	unlink(to);

	if (link(from, to) == -1)
	    return T_CREATERR;
    }

  remove_from:
    if (unlink(from) == -1)
	return F_DELETERR;

    return FT_OK;
}


/*
 * Return the index of the entry whose name is pointed to by `str', or 0
 * if no such entry is found.  Note that 0 is still a valid entry number,
 * but it is also used as the default entry in case of failure.
 */
int
panel_find_index(this, str)
    panel_t *this;
    char *str;
{
    int entry;
    size_t len = strlen(str);
    char *temp = xmalloc(len + 1);

    len = min(len, (size_t)this->maxname);
    strncpy(temp, str, len);
    temp[len] = 0;

    /* Do we have a perfect match?  */
    for (entry = 0; entry < this->entries; entry++)
	if (strcmp(temp, this->dir_entry[entry].name) == 0)
	    break;

    if (entry != this->entries)
	goto done;

    /* Do we have a case insensitive match?  */
    for (entry = 0; entry < this->entries; entry++)
	if (strcasecmp(temp, this->dir_entry[entry].name) == 0)
	    break;

    if (entry != this->entries)
	goto done;

    /* Do we have a partial match?  */
    for (entry = 0; entry < this->entries; entry++)
	if (strncmp(temp, this->dir_entry[entry].name,
		    min(len, strlen(this->dir_entry[entry].name))) == 0)
	    break;

    if (entry != this->entries)
	goto done;

    /* Do we have a partial case insensitive match?  */
    for (entry = 0; entry < this->entries; entry++)
	if (strncasecmp(temp, this->dir_entry[entry].name,
			min(len, strlen(this->dir_entry[entry].name))) == 0)
	    break;

    /* FIXME: We can add here as many heuristics as we want.  */

    if (entry == this->entries)
	entry = 0;

  done:
    xfree(temp);
    return entry;
}


/* An ugly hack.  */
extern int wait_msg;
extern char *screen;


/*
 * Change the directory of the current panel to the directory under
 * the cursor.  If that directory is ".." then go up to the parent
 * directory.
 */
int
panel_act_ENTER(this, other)
    panel_t *this, *other;
{
    int back;
    char *old_path, *cmd, *old_entry_name;
    char *name = this->dir_entry[this->current_entry].name;

    switch (this->dir_entry[this->current_entry].type)
    {
	case DIR_ENTRY:
	    if (strcmp(name, "..") == 0 && strcmp(this->path, "/") == 0)
		break;

	    back = (strcmp(name, "..") == 0) ? 1 : 0;

	    old_path = xmalloc(this->pathlen + 1);
	    strcpy(old_path, this->path);

	    if (!panel_read_directory(this, name, ON))
	    {
		if (back)
		{
		    panel_recover(this);
		    panel_update(this);
		}
		else
		    panel_2s_message("%s/: Permission denied.",
				     name, (char *)NULL,
				     IL_FREEZED | IL_BEEP |
				     IL_SAVE    | IL_ERROR);
		break;
	    }

	    if (back)
	    {
		int index;

		old_entry_name = strrchr(old_path, '/');
		assert(old_entry_name);
		index = panel_find_index(this, ++old_entry_name);
		panel_set_current_entry(this, index);
	    }
	    else
		panel_set_current_entry(this, 0);

	    xfree(old_path);

	    panel_update(this);

	    if (strcmp(this->path, other->path) == 0)
	    {
		panel_action(other, act_REGET, this, (void *)-1, 1);
		panel_update(other);
	    }
	    else
		panel_update_size(other);

	    break;

	case FILE_ENTRY:
	    if (this->dir_entry[this->current_entry].executable)
	    {
		cmd = xmalloc(32 + strlen(name) + 1);
		sprintf(cmd, "./\"%s\"", name);
		start(cmd, 0);
		wait_msg = 1;
		xfree(cmd);

		tty_touch();
		tty_get_screen(screen);
		panel_no_optimizations(this);
		panel_no_optimizations(other);
		il_insert_text(name);
		tty_update_title(this->path);
		return 1;
	    }

	    break;
	}

    return 0;
}


/*
 * Copy the selected files in the active panel into some other place,
 * usually the current directory of the other panel.
 */
void
panel_act_COPY(this, other)
    panel_t *this, *other;
{
    size_t len;
    int error, entry;
    char *file, *dir = NULL, *msg, *input = NULL, *tmp_input;


    this->chkdest = ON;

    if (this->selected_entries == 0)
    {
	char *name = this->dir_entry[this->current_entry].name;

	if (this->current_entry == 0 && !rootdir())
	    return;

	msg = xmalloc(16 + strlen(name) + 1);
	sprintf(msg, "Copy %s to: ", cutname(name, 0, 0));

	len  = 1 + strlen(name) + 1;
	file = xmalloc(strlen(other->path) + len);

	sprintf(file, "%s/%s", other->path, name);

	if (!il_read_line(msg, &input, file, copy_history))
	{
	    xfree(msg);
	    return;
	}

	xfree(msg);

	if (S_ISDIR(this->dir_entry[this->current_entry].mode))
	    il_message(PANEL_COPY_DIR_MSG);
	else
	    il_message(PANEL_COPY_FILE_MSG);

	tty_update();
	tmp_input = tilde_expand(input);
	xfree(input);
	input = tmp_input;

	error = same_file(name, input);
	xfree(file);

	if (error)
	{
	    panel_3s_message("%s and %s point to the same file.",
			     name, input, (char *)NULL,
			     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
	    xfree(input);
	    return;
	}

	error = panel_copy(this, name, input,
			   this->dir_entry[this->current_entry].mode,
			   getuid(), getgid());

	xfree(input);

	if (error != SD_OK && error != SD_CANCEL)
	    panel_3s_message("%s: Copy failed, %s.", name,
			     copyerr[error - 1], (char *)NULL,
			     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);

	status_default();
	panel_update_size(this);
	panel_update_size(other);
	tty_update();
    }
    else
    {
	if (!il_read_line("Copy selected file(s) to: ", &dir,
			  other->path, copy_history))
	    return;

	if (same_file(this->path, dir))
	{
	    panel_1s_message(nice_try, (char *)NULL,
			     IL_FREEZED | IL_BEEP | IL_ERROR);
	    return;
	}

	dir = xrealloc(dir, (len = strlen(dir) + 1) + 1);
	dir[len-1] = '/';
	dir[len  ] = '\0';

	panel_init_iterator(this);

	while ((entry = panel_get_next(this)) != -1)
	{
	    char *name  = this->dir_entry[entry].name;
	    mode_t mode = this->dir_entry[entry].mode;
	    uid_t uid = getuid();
	    gid_t gid = getgid();

	    dir = xrealloc(dir, len + strlen(name) + 1);
	    strcpy(dir + len, name);

	    if (canceled())
		break;

	    il_message(PANEL_COPY_FILES_MSG);
	    tty_update();

	    error = panel_copy(this, name, dir, mode, uid, gid);

	    if (error != SD_OK)
	    {
		if (error == SD_CANCEL)
		    break;

		if (error == SD_SKIP)
		    continue;

		if (panel_3s_message("%s: Copy failed, %s.",
				     name, copyerr[error - 1], (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_ERROR) == 0)
		    break;
	    }
	    else
		this->dir_entry[entry].selected = 0;

	    panel_update_size(this);
	    panel_update_size(other);
	}

	if (dir)
	    xfree(dir);

	status_default();
	tty_update();
    }

    if (!panel_read_directory(other, other->path, ON))
	panel_recover(other);

    panel_update(other);

    if (!panel_read_directory(this, this->path, ON))
	panel_recover(this);

    panel_update(this);
}


/*
 * Delete the selected files and directories in the active panel.
 */
void
panel_act_DELETE(this, other)
    panel_t *this, *other;
{
    char *msg;
    char *command;
    int keep_asking = 1;
    int first_entry, entry, answer = 0, result;


    if (this->selected_entries == 0 &&
	(this->current_entry == 0 && !rootdir()))
	return;

    if (panel_1s_message("Delete selected entries? ","yn",IL_FREEZED) != 'y')
	return;

    /* Remember the index of the first selected file.  */
    for (first_entry = 0; first_entry < this->entries; first_entry++)
	if (this->dir_entry[first_entry].selected)
	    break;

    panel_init_iterator(this);

    while ((entry = panel_get_next(this)) != -1)
    {
	char *name = this->dir_entry[entry].name;
	int interrupted=0;

	service_pending_signals();

	msg = xmalloc(32 + strlen(name));
	sprintf(msg, "(DELETE) %s", name);
	status(msg, STATUS_ERROR, STATUS_LEFT);
	tty_update();
	xfree(msg);

	if (canceled())
	    break;

	if (keep_asking)
	    answer = panel_2s_message("Delete %s? (Yes/Skip/All/Cancel) ",
				      name, "ysac", IL_MOVE);

	il_message(PANEL_DELETE_FILES_MSG);
	tty_update();

	if (answer == 'a')
	    keep_asking = 0;
	else if (answer == 's')
	    continue;
	else if (answer == 'c')
	    break;
	else if (answer != 'y')
	    break;

	if (this->dir_entry[entry].type == DIR_ENTRY)
	{
	    il_message(PANEL_DELETE_DIR_MSG);
	    tty_update();
	    result = (rmdir(name) == 0);

	    if (!result)
	    {
		if (panel_2s_message(
			"%s/: directory might contain files.  Delete? ",
			name, "yn", IL_MOVE | IL_SAVE) == 'y')
		{
		    command = xmalloc(32 + strlen(name) + 1);
		    sprintf(command, "rm -r -f \"%s\"", name);
		    result = start(command, 1);
		    xfree(command);

		    tty_update_title(this->path);

		    if(WIFSIGNALED(result))
		    {
			result=0;
			interrupted=1;
		    }
		    else if(WIFEXITED(result))
		    {
			if(WEXITSTATUS(result) != 0)
			{
			    display_errors("rm");
			    result=0;
			}
			else
			{
			    result=1; /* ok */
			}
		    }
		    else
			result=0; /* unknown error */
		}
	    }
	}
	else
	    result = unlink(name) == 0;

	if (interrupted)
	{
	    if (panel_2s_message("%s: Deletion interrupted.  Continue? ",
				 name, "yn",
				 IL_MOVE | IL_BEEP | IL_ERROR) != 'y')
		break;
	}
	else if (!result)
	{
	    if (panel_2s_message("%s: Deletion failed.  Continue? ",
				 name, "yn",
				 IL_MOVE | IL_BEEP | IL_ERROR) != 'y')
		break;
	}
	else
	    this->dir_entry[entry].selected = 0;
    }

    if (first_entry != this->entries)
	panel_set_current_entry(this, first_entry);

    panel_update_size(this);
    panel_update_size(other);
    status_default();
    tty_update();

    if (!panel_read_directory(this, this->path, ON))
	panel_recover(this);

    panel_update(this);

    if (strcmp(this->path, other->path) == 0)
    {
	if (!panel_read_directory(other, other->path, ON))
	    panel_recover(other);

	panel_update(other);
    }
}


int
panel_isdir(path)
    const char *path;
{
    struct stat s;

    if (stat(path, &s) < 0)
	return 0;

    return S_ISDIR(s.st_mode);
}


int
panel_mkdirs(path, mode)
    const char *path;
    int mode;
{
    char *copy = xstrdup(path);
    char *z = copy;

    for (; *z != '\0'; z++)
	if (*z == '/' && z != copy)
	{
	    *z = '\0';
	    if (!panel_isdir(copy))
		if (mkdir(copy, mode) != 0)
		{
		    xfree(copy);
		    return -1;
		}
	    *z = '/';
	}

    xfree(copy);
    return 0;
}


/*
 * Create a new directory into the current panel's directory.
 */
void
panel_act_MKDIR(this, other)
    panel_t *this, *other;
{
    size_t len;
    char *input = NULL, *tmp_input;

    if (!il_read_line("New directory name: ", &input, NULL, mkdir_history))
	return;

    if (input[0] == '\0')
    {
	xfree(input);
	return;
    }

    tmp_input = tilde_expand(input);
    xfree(input);
    input = tmp_input;
    /* Add a '/' at the end, otherwise panel_mkdirs() will think the
       last component is a regular file and will not create the
       directory.  */
    len = strlen(input);
    input = realloc(input, len + 1 + 1);
    input[len] = '/';
    input[len + 1] = '\0';

    /* I don't remember why I've put S_IFDIR here.  Is it really
       necessary?  */
    if (panel_mkdirs(input, S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO) == -1)
    {
	panel_2s_message("%s: Permission denied.", input,
			 (char *)NULL,
			 IL_FREEZED | IL_BEEP | IL_SAVE | IL_ERROR);
	xfree(input);
	return;
    }

    if (!panel_read_directory(this, this->path, ON))
	panel_recover(this);
    else
    {
	/* If `input' is something like `a/b/c' we will default on the
	   first entry.  */
	this->current_entry = panel_find_index(this, input);
	this->first_on_screen = panel_get_centered_fos(this);
	panel_update_entries(this);
	panel_update_info(this);
	panel_update_size(this);
    }

    if (strcmp(this->path, other->path) == 0)
    {
	char *old_entry = xstrdup(other->dir_entry[other->current_entry].name);

	if (!panel_read_directory(other, other->path, ON))
	    panel_recover(other);
	else
	{
	    other->current_entry = panel_find_index(other, old_entry);
	    other->first_on_screen = panel_get_centered_fos(other);
	    panel_update_entries(other);
	    panel_update_info(other);
	}

	xfree(old_entry);
    }

    panel_update_size(other);
    xfree(input);
}


/*
 * Move the selected files and directories from the active panel into
 * some other place in the current file system (usually in the other
 * panel's directory.  Files can also be moved across file systems.
 */
void
panel_act_MOVE(this, other)
    panel_t *this;
    panel_t *other;
{
    size_t len;
    int first_entry, entry, error;
    char *file, *dir = NULL, *msg, *input = NULL, *tmp_input;


    this->chkdest = ON;

    if (this->selected_entries == 0)
    {
	char *name = this->dir_entry[this->current_entry].name;

	if (this->current_entry == 0 && !rootdir())
	    return;

	msg = xmalloc(16 + strlen(name) + 1);
	sprintf(msg, "Move %s to: ", cutname(name, 0, 0));

	len  = 1 + strlen(name) + 1;
	file = xmalloc(strlen(other->path) + len);

	sprintf(file, "%s/%s", other->path, name);

	if (!il_read_line(msg, &input, file, move_history))
	{
	    xfree(msg);
	    return;
	}

	xfree(msg);

	if (S_ISDIR(this->dir_entry[this->current_entry].mode))
	    il_message(PANEL_MOVE_DIR_MSG);
	else
	    il_message(PANEL_MOVE_FILE_MSG);

	tty_update();
	tmp_input = tilde_expand(input);
	xfree(input);
	input = tmp_input;

	error = same_file(name, input);
	xfree(file);

	if (error)
	{
	    panel_3s_message("%s and %s point to the same file.",
			     name, input, (char *)NULL,
			     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
	    xfree(input);
	    return;
	}

	error = panel_move(this, name, input,
			   this->dir_entry[this->current_entry].mode);

	if (error != FT_OK)
	{
	    if (error == FT_CANCEL)
	    {
		xfree(input);
		status_default();
		tty_update();
		return;
	    }

	    panel_3s_message("%s: Move failed, %s.", name, moveerr[error - 1],
			     (char *)NULL, IL_MOVE | IL_BEEP | IL_ERROR);
	}

	xfree(input);
	status_default();
	panel_update_size(this);
	panel_update_size(other);
	tty_update();
    }
    else
    {
	if (!il_read_line("Move selected file(s) to: ", &dir,
			  other->path, move_history))
	    return;

	if (same_file(this->path, dir))
	{
	    panel_1s_message(nice_try, (char *)NULL,
			     IL_FREEZED | IL_BEEP | IL_ERROR);
	    return;
	}

	dir = xrealloc(dir, (len = strlen(dir) + 1) + 1);
	dir[len-1] = '/';
	dir[len  ] = '\0';

	/* Remember the index of the first selected file.  */
	for (first_entry = 0; first_entry < this->entries; first_entry++)
	    if (this->dir_entry[first_entry].selected)
		break;

	panel_init_iterator(this);

	while ((entry = panel_get_next(this)) != -1)
	{
	    char *name = this->dir_entry[entry].name;

	    service_pending_signals();

	    dir = xrealloc(dir, len + strlen(name) + 1);
	    strcpy(dir + len, name);

	    if (canceled())
		break;

	    il_message(PANEL_MOVE_FILES_MSG);
	    tty_update();

	    error = panel_move(this, name, dir, this->dir_entry[entry].mode);

	    if (error != FT_OK)
	    {
		if (error == FT_CANCEL)
		    break;

		if (error == FT_SKIP)
		    continue;

		if (panel_3s_message("%s: Move failed, %s.", name,
				     moveerr[error - 1], (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_ERROR) == 0)
		    break;
	    }
	    else
		this->dir_entry[entry].selected = 0;
	}

	if (dir)
	    xfree(dir);

	if (first_entry != this->entries)
	    panel_set_current_entry(this, first_entry);

	status_default();
	tty_update();
    }

    if (!panel_read_directory(other, other->path, ON))
	panel_recover(other);

    panel_update(other);

    if (!panel_read_directory(this, this->path, ON))
	panel_recover(this);

    panel_update(this);
}


/*
 * Change the directory of the current panel.  Refresh the other panel as
 * well if it turns out that both point to the same directory.
 */
void
panel_act_CHDIR(this, other, new_dir)
    panel_t *this;
    panel_t *other;
    char *new_dir;
{
    assert(new_dir);

    if (new_dir[0] == '/')
    {
	this->pathlen = strlen(new_dir);
	this->path = xrealloc(this->path, (this->pathlen + 1) * sizeof(char));
	strcpy(this->path, new_dir);
    }
    else
    {
	this->pathlen += 1 + strlen(new_dir);
	this->path = xrealloc(this->path, (this->pathlen + 1) * sizeof(char));
	strcat(this->path, "/");
	strcat(this->path, new_dir);
	minimize_path(this->path);
    }

    panel_set_current_entry(this, 0);
    panel_action(this, act_REGET, (panel_t *)NULL, (void *)NULL, 1);
    panel_update(this);

    if (strcmp(this->path, other->path) == 0)
    {
	panel_action(other, act_REGET, (panel_t *)NULL, (void *)-1, 1);
	panel_update(other);
    }
}


/*
 * Refresh a panel by re-reading the directory from the disk.
 */
void
panel_act_REGET(this, aux_info)
    panel_t *this;
    void *aux_info;
{
    char *old_entry;
    int flag, verify;

    if (this->dir_entry && this->dir_entry[this->current_entry].name)
    {
	old_entry = xstrdup(this->dir_entry[this->current_entry].name);
	flag = 1;
    }
    else
	old_entry = "", flag = 0;

    verify = aux_info == (void *)-1;

    if (panel_read_directory(this, this->path, verify))
    {
	if (verify)
	    panel_set_current_entry(this, panel_find_index(this, old_entry));
	else
	    panel_set_current_entry(this, 0);
    }
    else
	panel_recover(this);

    if (flag)
	xfree(old_entry);
}


#define CMP_BUFFER_SIZE (32 * 1024)

#define CF_ABORT	-1
#define CF_READ1	-2
#define CF_READ2	-3
#define CF_OPEN1	-4
#define CF_OPEN2	-5


/*
 * Compare the file at index `this_entry' into `this' with the file at
 * index `other_entry' into `other'. File sizes are written into
 * *this_size and *other_size.
 *
 * Return:
 *   CF_ABORT if aborted,
 *   CF_READ1 if there was a read error on the first file,
 *   CF_READ2 if there was a read error on the second file,
 *   CF_OPEN1 if couldn't open the first file,
 *   CF_OPEN2 if couldn't open the second file,
 * or return the offset of the first difference encountered.
 *
 */
off64_t
panel_compare(this, this_entry, this_size, other, other_entry, other_size)
    panel_t *this;
    int this_entry;
    off64_t *this_size;
    panel_t *other;
    int other_entry;
    off64_t *other_size;
{
    off64_t n;
    char *msg;
    int fd1, fd2;
    char *buf1, *buf2;
    int read1, read2;
    int aborted = 0;
    char *name1 = this->dir_entry[this_entry].name;
    char *name2 = other->dir_entry[other_entry].name;
    char *file1 = name1;
    char *file2;
    int is_special1=0;
    int is_special2=0;
    int mismatch=0;
    off64_t size=0;

    *this_size = *other_size = 0;

    if(IS_SPECIAL(this->dir_entry[this_entry].mode))
	is_special1=1;
    if(IS_SPECIAL(other->dir_entry[this_entry].mode))
	is_special2=1;

    /* If either is a special file, use the length of the other. */
    /* size is only used for calculating percentage done */
    if(is_special1 && is_special2)
	size=0;
    else if(is_special1)
	size=(off64_t)other->dir_entry[other_entry].size;
    else if(is_special2)
	size=(off64_t)this->dir_entry[this_entry].size;
    else /* neither are special files */
	size=(off64_t)max(this->dir_entry[this_entry].size,
			  other->dir_entry[other_entry].size);
    if ((size == 0) && !(is_special1&&is_special2))
	return 0;

    file2 = xmalloc(strlen(other->path) + 1 + strlen(name2) + 1);
    sprintf(file2, "%s/%s", other->path, name2);

    msg = xmalloc(32 + strlen(file1) + 1);
    if(!size)
	sprintf(msg, "(CMP) %s", file1);
    else
	sprintf(msg, "(CMP) [  0%%] %s", file1);
    status(msg, STATUS_WARNING, STATUS_LEFT);
    tty_update();
    xfree(msg);

    if ((fd1 = open64(file1, O_RDONLY | O_BINARY)) == -1)
	return CF_OPEN1;

    if ((fd2 = open64(file2, O_RDONLY | O_BINARY)) == -1)
    {
	xfree(file2);
	close(fd1);
	return CF_OPEN2;
    }

    xfree(file2);

    buf1 = xmalloc(CMP_BUFFER_SIZE);
    buf2 = xmalloc(CMP_BUFFER_SIZE);

    msg = xmalloc(32 + strlen(file1) + 1);

    for(n=0 ; ; )
    {
	int bytes_read;

	if (canceled())
	{
	    aborted = 1;
	    break;
	}

	signals(ON);
	read1 = read(fd1, buf1, CMP_BUFFER_SIZE);
	signals(OFF);

	if (read1 < 0)
	{
	    close(fd1);
	    close(fd2);
	    return CF_READ1;
	}
	*this_size += read1;

	signals(ON);
	read2 = read(fd2, buf2, CMP_BUFFER_SIZE);
	signals(OFF);

	if (read2 < 0)
	{
	    close(fd1);
	    close(fd2);
	    return CF_READ2;
	}
	*other_size += read2;

	bytes_read = min(read1, read2);

	if(bytes_read == 0)
	    break;

	if (memcmp(buf1, buf2, bytes_read) != 0)
	{
	    int i;

	    /* Ok, there is a difference somewhere.  Let's go find it.  */
	    for (i = 0; i < bytes_read; i++)
		if (buf1[i] != buf2[i])
		    break;

	    n += i;
	    mismatch=1;
	    break;
	}

	n += bytes_read;

	if(!size)
	    sprintf(msg, "(CMP) %s", file1);
	else
	    sprintf(msg, "(CMP) [%3d%%] %s",
		    panel_percent(n, size), file1);
	status(msg, STATUS_WARNING, STATUS_LEFT);
	tty_update();
    }

    if(mismatch)
    {
	/* we aborted the compare before eof. We need to make sure
	   this_size and other_size are right. If they are special files,
	   we need to keep reading to get the size */
	if(is_special1)
	{
	    signals(ON);
	    do
	    {
		read1 = read(fd1, buf1, CMP_BUFFER_SIZE);
	    } while((read1 > 0) && (*this_size += read1));
	    signals(OFF);
	}
	else
	    *this_size=this->dir_entry[this_entry].size;

	if(is_special2)
	{
	    signals(ON);
	    do
	    {
		read2 = read(fd2, buf2, CMP_BUFFER_SIZE);
	    } while((read2 > 0) && (*other_size += read2));
	    signals(OFF);
	}
	else
	    *other_size=other->dir_entry[other_entry].size;
    }

    xfree(msg);

    xfree(buf1);
    xfree(buf2);

    close(fd1);
    close(fd2);

    return aborted ? CF_ABORT : n;
}


/*
 * Compare the current file in the active panel with the current
 * file in the other one.
 */
void
panel_act_COMPARE(this, other)
    panel_t *this;
    panel_t *other;
{
    int permission = 1;
    int this_entry = this->current_entry;
    int other_entry = other->current_entry;

    il_message(PANEL_COMPARE_FILES_MSG);
    tty_update();

    if ((strcmp(this->path, other->path) == 0) &&
	(strcmp(this->dir_entry[this_entry].name,
		other->dir_entry[other_entry].name) == 0))
    {
	panel_1s_message("There is no point in comparing a file with itself. ",
			 (char *)NULL, IL_BEEP | IL_SAVE | IL_ERROR);
	return;
    }

    if (this->dir_entry[this_entry].type == FILE_ENTRY &&
	other->dir_entry[other_entry].type == FILE_ENTRY)
    {
	if ((this->dir_entry[this_entry].size !=
	     other->dir_entry[other_entry].size) &&
	    (!(IS_SPECIAL(this->dir_entry[this_entry].mode) ||
	       IS_SPECIAL(other->dir_entry[other_entry].mode))))
	{
	    /* Ask for permission to continue if the files have
	       different size.  */
	    if (panel_1s_message("Files have different size.  Continue? ",
				 "yn", IL_BEEP | IL_ERROR) != 'y')
		permission = 0;
	}

	if (permission)
	{
	    off64_t this_size,other_size;
	    off64_t result =
                panel_compare(this, this_entry, &this_size, other, other_entry, &other_size);

	    switch ((int)result)
	    {
		case CF_ABORT:
		    break;

		case CF_OPEN1:
		    panel_2s_message("Cannot open file %s. ",
				     this->dir_entry[this_entry].name,
				     (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
		    break;

		case CF_OPEN2:
		    panel_2s_message("Cannot open file %s. ",
				     other->dir_entry[other_entry].name,
				     (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
		    break;

		case CF_READ1:
		    panel_2s_message("I/O error reading from file %s. ",
				     this->dir_entry[this_entry].name,
				     (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
		    break;

		case CF_READ2:
		    panel_2s_message("I/O error reading from file %s. ",
				     other->dir_entry[other_entry].name,
				     (char *)NULL,
				     IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
		    break;

		default:
		    if ((result == this_size) && (this_size == other_size))
		    {
			/* files are identical */
			panel_1s_message("Compare OK. ", (char *)NULL,
					 IL_BEEP | IL_SAVE);
		    }
		    else if (result == min(this_size, other_size))
		    {
			/* The files appear to be equal, if we ignore
			   the difference in size.  */
			panel_1s_message("Files are different sizes but are identical up to the size of the smallest.",
					 (char *)NULL,
					 IL_BEEP | IL_SAVE);
		    }
		    else
		    {
			/* The files differ.  Report the offset of the
			   first difference encountered.  */
			char *msg = xmalloc(128);

#ifdef HAVE_64BIT_IO
			sprintf(msg, "%Ld (0x%Lx)", result, result);
#else /* !HAVE_64BIT_IO */
			sprintf(msg, "%ld (0x%lx)",
				(long)result, (long)result);
#endif /* !HAVE_64BIT_IO */
			panel_2s_message("Files differ at offset %s. ",
					 msg, (char *)NULL, IL_BEEP | IL_SAVE);
			xfree(msg);
		    }

		    break;
	    }
	}
    }
    else
    {
	panel_2s_message("Only regular files can be compared. ",
			 this->path, (char *)NULL,
			 IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
	/* At least one of the files is not a regular file, thus we
	   will not go any further.  */
    }

    status_default();
    tty_update();
}


/*
 * Compare the files in the two panels.  If `quick' is 1, then perform
 * a "quick" comparison, i.e. just look at the name, size and date of
 * the files.  Otherwise, do a thorough comparison.
 */
void
panel_act_CMPDIR(this, other, quick)
    panel_t *this;
    panel_t *other;
    int quick;
{
    int i, j;

    il_message(PANEL_COMPARE_DIR_MSG);
    tty_update();

    if (strcmp(this->path, other->path) == 0)
    {
	panel_1s_message("No point in comparing a directory with itself. ",
			 (char *)NULL, IL_BEEP | IL_SAVE | IL_ERROR);

	panel_unselect_all(this);
	panel_unselect_all(other);

	panel_update(this);
	panel_update(other);

	return;
    }

    panel_select_all(this);
    panel_select_all(other);

    for (i = 1; i < this->entries; i++)
	if (this->dir_entry[i].type == FILE_ENTRY)
	    for (j = 1; j < other->entries; j++)
	    {
		service_pending_signals();

		if (other->dir_entry[j].type == FILE_ENTRY &&
		    strcmp(this->dir_entry[i].name,
			   other->dir_entry[j].name) == 0)
		{
		    if (this->dir_entry[i].size == other->dir_entry[j].size)
		    {
			if (quick)
			{
			    if (this->dir_entry[i].mtime !=
				other->dir_entry[j].mtime)
				goto hilight_the_newer_one;
			    else
				goto unhilight_both;
			}
			else
			{
			    off64_t this_size, other_size;
			    off64_t result = panel_compare(this,  i, &this_size,
							   other, j, &other_size);

			    if (result == CF_ABORT)
				goto done;

			    if ((result == this_size) &&
				(result == other_size))
				goto unhilight_both;
			}
		    }

		  hilight_the_newer_one:
		    /* We don't use difftime(), to avoid floating
		       point (Thix doesn't have fp :-().  Not good.  */
		    if (this->dir_entry[i].mtime >= other->dir_entry[j].mtime)
		    {
			other->dir_entry[j].selected = 0;
			other->selected_entries--;
		    }
		    else
		    {
			this->dir_entry[i].selected = 0;
			this->selected_entries--;
		    }
		    break;

		  unhilight_both:
		    this->dir_entry[i].selected = 0;
		    this->selected_entries--;
		    other->dir_entry[j].selected = 0;
		    other->selected_entries--;
		    break;
		}
	    }

  done:
    status_default();
    panel_update(this);
    panel_update(other);
    tty_update();

    /* Beep if there are differences.  */
    if (this->selected_entries || other->selected_entries)
	tty_beep();
}


#define ON_OK		WARN_OK
#define ON_CANCEL	WARN_CANCEL
#define ON_SKIP		WARN_SKIP
#define O_RMERR		4
#define N_RMERR		5
#define ON_RENERR	6
#define ON_INTERRUPTED  7

char *renerr[7] =
{
    "",
    "",
    "",
    "cannot remove old entry",
    "cannot remove existing entry",
    "cannot rename entry",
    "mv interrupted by a signal",
};


int
panel_case_rename(this, entry, upcase)
    panel_t *this;
    int entry;
    int upcase;
{
    char *msg;
    char *new_name;
    int n = 0, error, first_time = 1, len;
    char *name = this->dir_entry[entry].name;

    /* First compute the new name of the entry.  */
    new_name = xstrdup(name);
    len = strlen(name);

    if (upcase)
	for (n = 0; n < len; n++)
	    new_name[n] = toupper((int)name[n]);
    else
	for (n = 0; n < len; n++)
	    new_name[n] = tolower((int)name[n]);

    /* If the name has not been changed, skip this entry.  */
    if (strcmp(name, new_name) == 0)
	goto done;

    msg = xmalloc(32 + len + 1);
    sprintf(msg, "(CASE) Renaming %s", name);
    status(msg, STATUS_WARNING, STATUS_LEFT);
    tty_update();
    xfree(msg);

    if (this->dir_entry[entry].type == DIR_ENTRY)
    {
	if (access(new_name, F_OK) == 0)
	{
	    int result;
	    char *command;

	    /* The destination name (new_name) exists.  Ask permission
	       to overwrite.  */
	    if (this->chkdest == ON)
		error = panel_warning(this, new_name);
	    else
		error = WARN_OVERWRITE;

	    if (error)
		return error;

	    command = xmalloc(32 + strlen(new_name) + 1);
	    sprintf(command, "rm -r -f \"%s\"", new_name);
	    result = start(command, 1);
	    xfree(command);

	    tty_update_title(this->path);

	    if (WIFSIGNALED(result))
	    {
		return ON_INTERRUPTED;
	    }
	    if(WIFEXITED(result))
	    {
		if(WEXITSTATUS(result) != 0)
		{
		    display_errors("rm");
		    return N_RMERR;
		}
	    }
	    else
	    {
		return N_RMERR;
	    }
	}

	if (rename(name, new_name) == 0)
	    goto done;
	else
	    return ON_RENERR;
    }

    /* Files only.  */
  retry:
    if (link(name, new_name) == 0)
    {
	if (unlink(name) == 0)
	    goto done;
	else
	    return O_RMERR;
    }
    else
    {
	int safe_errno = errno;

	status_default();
	tty_update();

	if (safe_errno == EPERM || safe_errno == EACCES)
	    return ON_RENERR;

	if (!first_time)
	    return ON_RENERR;

	if (this->chkdest == ON)
	    error = panel_warning(this, new_name);
	else
	    error = WARN_OVERWRITE;

	if (error)
	    return error;

	if (unlink(new_name) != 0)
	    return N_RMERR;

	if (first_time)
	{
	    first_time = 0;
	    goto retry;
	}
    }

  done:
    this->dir_entry[entry].selected = 0;
    this->selected_entries--;

    return ON_OK;
}


void
panel_act_CASE(this, other, upcase)
    panel_t *this;
    panel_t *other;
    int upcase;
{
    int entry, error;

    il_message(PANEL_RENAME_FILES_MSG);
    tty_update();

    this->chkdest = ON;

    panel_init_iterator(this);

    while ((entry = panel_get_next(this)) != -1)
    {
	service_pending_signals();

	error = panel_case_rename(this, entry, upcase);

	if (error != ON_OK)
	{
	    if (error == ON_CANCEL)
		break;

	    if (error == ON_SKIP)
		continue;

	    if (error != ON_OK && error != ON_CANCEL)
		panel_3s_message("%s: Rename failed, %s.",
				 this->dir_entry[entry].name,
				 renerr[error - 1], (char *)NULL,
				 IL_MOVE | IL_BEEP | IL_SAVE | IL_ERROR);
	}
	else
	    this->dir_entry[entry].selected = 0;
    }

    status_default();
    tty_update();

    panel_action(this, act_REGET, (panel_t *)NULL, (void *)NULL, 1);
    panel_update(this);

    if (strcmp(this->path, other->path) == 0)
    {
	panel_action(other, act_REGET, (panel_t *)NULL, (void *)NULL, 1);
	panel_update(other);
    }
}


/*
 * Only regular files are considered.  The other files are ignored.
 */
static int
pack_compare_fn(first, second)
    const void *first;
    const void *second;
{
    return (*(const dir_entry_t * const *)second)->size -
	   (*(const dir_entry_t * const *)first)->size;
}


/*
 * Select the files that would go into the first bin according to the
 * "First Fit Decreasing" aproximation for the bin packing algorithm.
 * This algorithm is just a poor aproximation, first because the
 * problem of finding an optimal solution for bin-packing is
 * NP-complete (with ((11 / 9) * OPTIMAL + 4) being the best known
 * aproximation guarantee, and second because we don't actually know
 * how big a file system block actually is.  Therefore we don't know
 * if we should round up file sizes to 512 bytes, 1Kb, 2Kb, 4Kb, etc.
 * We will assume 1Kb, but it might not necessary be true...
 */
void
panel_act_BIN_PACKING(this, other, bin_size)
    panel_t *this;
    panel_t *other;
    off64_t bin_size;
{
    char msg[160];
    off64_t file_size;
    long free_blocks;
    off64_t *bins = NULL;
    dir_entry_t **buffer;
    char *fn = "BIN PACKING";
    int max_bins = 0, used_bins = 0;
    int entry, candidates = 0, big_files = 0;

    if (bin_size == 0)
    {
	struct fs_usage fsu;

	fsu.fsu_blocks = (uintmax_t) -1;

	/* get_fs_usage will fail on SVR2 (needs disk instead of NULL) */

	if (get_fs_usage(other->path, NULL, &fsu) >= 0 && fsu.fsu_blocks != (uintmax_t) -1)
	{
	    /* Make bin_size equal the file system free space in the
	       other panel.  */
	    free_blocks = ((geteuid() == 0) ? fsu.fsu_bfree : fsu.fsu_bavail);
	    bin_size = (free_blocks * fsu.fsu_blocksize) / 1024;
	}
    }

    /* First unselect all the files in the current panel.  */
    panel_unselect_all(this);

    /* Create a vector of dir_entry_t pointers and fill it with
       pointers to the directory entries that are regular files.
       Note that we skip the regular files whose size is bigger
       than the bin size.  */
    buffer = (dir_entry_t **)xmalloc(this->entries * sizeof(dir_entry_t *));

    for (entry = 0; entry < this->entries; entry++)
	if (S_ISREG(this->dir_entry[entry].mode))
	{
	    file_size = this->dir_entry[entry].size;

	    if (file_size % 1024)
		file_size += 1024 - file_size % 1024;

	    file_size /= 1024;

	    if (file_size <= bin_size)
		buffer[candidates++] = &this->dir_entry[entry];
	    else
		big_files++;
	}

    /* Then sort the pointers in decreasing order, based on the sizes
       of the corresponding directory entries.  */
    qsort(buffer, candidates, sizeof(dir_entry_t *), pack_compare_fn);

    /* Now start filling the bins.  Assume an infinite number of bins.  */
    for (entry = 0; entry < candidates; entry++)
    {
	int current_bin;
	file_size = buffer[entry]->size;

	if (file_size % 1024)
	    file_size += 1024 - file_size % 1024;

	file_size /= 1024;

	/* Look for a suitable bin.  */
	for (current_bin = 0; current_bin < used_bins; current_bin++)
	    if (bins[current_bin] >= file_size)
	    {
		bins[current_bin] -= file_size;

		/* Select the file if it goes into the first bin.  */
		if (current_bin == 0)
		{
		    buffer[entry]->selected = 1;
		    this->selected_entries++;
		}

		break;
	    }

	if (current_bin == used_bins)
	{
	    /* The file doesn't fit into any of the bins.  We have to
	       create a new bin.  Put the file into the newly created
	       bin.  */
	    if (used_bins == max_bins)
	    {
		max_bins += 16;
		bins = (off64_t *)xrealloc(bins, max_bins * sizeof(off64_t));
	    }

	    bins[used_bins++] = bin_size - file_size;

	    if (current_bin == 0)
	    {
		buffer[entry]->selected = 1;
		this->selected_entries++;
	    }
	}
    }

    panel_update(this);

    if (bins)
	xfree(bins);

    xfree(buffer);

    if (candidates > 0)
    {
	if (big_files > 0)
	{
	    sprintf(msg,
		    "%s %d file(s): %s %d bin(s).  %d file(s) are too big.",
		    fn, candidates, "You need approximately",
		    used_bins, big_files);
	}
	else
	    sprintf(msg,
		    "%s %d file(s): You need approximately %d bin(s).",
		    fn, candidates, used_bins);
    }
    else
    {
	if (big_files > 0)
	    sprintf(msg, "%s: No suitable files found (smaller than %ldKb).",
		    fn, (long)bin_size);
	else
	    sprintf(msg, "%s: No regular files found.", fn);
    }

    panel_1s_message(msg, (char *)NULL,
		     IL_FREEZED | IL_BEEP | IL_SAVE | IL_MOVE | IL_HOME);
}


/*
 * Create a vector of pointers to strings, one string for each pattern
 * in `string'.  The patterns in string are separated by spaces.
 * Spaces can be inserted into patterns by quoting them with a \.  The
 * \ itself can be used if inserted twice.
 */
char **
panel_parse_patterns(string)
    char *string;
{
    char c;
    int i = 0;
    int index = 0;
    int escaping = 0;
    char *pattern = xmalloc(strlen(string) + 1);
    char **patterns = (char **)xcalloc(2, sizeof(char *));

    while (1)
	switch ((c = *string++))
	{
	    case '\0':
	    case ' ':
	    case '\t': if (escaping && c != '\0')
			   pattern[i++] = c;
		       else
		       {
			   if (i > 0)
			   {
			       /* The pattern is not empty.  Add it to
				  the list.  */
			       pattern[i++] = '\0';
			       patterns[index++] = xstrdup(pattern);
			       patterns[index] = NULL;
			   }

			   if (c == '\0')
			       goto done;

			   patterns = (char **)xrealloc(patterns,
							(index + 2) *
							sizeof(char *));
			   i = 0;
		       }
		       break;

	    case '\\': if (escaping)
			   pattern[i++] = c;
		       escaping = !escaping;
		       break;

	    default:   if (escaping)
			   pattern[i++] = '\\';
		       pattern[i++] = c;
		       escaping = 0;
		       break;
	}

  done:
    xfree(pattern);
    return patterns;
}


void
panel_deallocate_patterns(patterns)
    char **patterns;
{
    char **safe_patterns = patterns;

    while (*patterns)
	xfree(*patterns++);

    xfree(safe_patterns);
}


int
panel_action(this, action, other, aux_info, repeat_count)
    panel_t *this;
    int action;
    panel_t *other;
    void *aux_info;
    int repeat_count;
{
    char *ptr;
    size_t len;
    char *previous;
    char *extension;
    isearch_aux_t *iai;
    int new_scroll_step;
    char **patterns, **safe_patterns;
    int entry, result, max_offset;
    int need_update, need_update_all, old_current_entry;

    switch (action)
    {
	case act_ENTER:
	    return panel_act_ENTER(this, other);

	case act_COPY:
	    panel_act_COPY(this, other);
	    break;

	case act_DELETE:
	    panel_act_DELETE(this, other);
	    break;

	case act_SELECT:
	    /* In the root directory there is no '..' entry, so the first
	       entry can be selected.  Avoid selecting the '..' directory
	       entry in a normal directory.  */
	    if (rootdir() || this->current_entry != 0)
	    {
		this->dir_entry[this->current_entry].selected =
		    !this->dir_entry[this->current_entry].selected;
		this->selected_entries +=
		    this->dir_entry[this->current_entry].selected ? 1 : -1;
		panel_update_entry(this, this->current_entry);
	    }

	    panel_action(this, act_DOWN, other, (void *)NULL, repeat_count);
	    break;

	case act_SELECT_ALL:
	    panel_select_all(this);
	    panel_update_entries(this);
	    panel_update_info(this);
	    break;

	case act_UNSELECT_ALL:
	    panel_unselect_all(this);
	    panel_update_entries(this);
	    panel_update_info(this);
	    break;

	case act_TOGGLE:
	    this->selected_entries = 0;

	    for (entry = 0; entry < this->entries; entry++)
		if (this->dir_entry[entry].type != DIR_ENTRY)
		{
		    this->dir_entry[entry].selected =
			!this->dir_entry[entry].selected;
		    this->selected_entries += this->dir_entry[entry].selected;
		}

	    panel_update_entries(this);
	    panel_update_info(this);
	    break;

	case act_MKDIR:
	    panel_act_MKDIR(this, other);
	    break;

	case act_MOVE:
	    panel_act_MOVE(this, other);
	    break;

	case act_UP:
	    need_update_all = need_update = 0;

	    while (repeat_count--)
	    {
		if (this->current_entry != 0)
		    this->current_entry--;
		else
		    break;

		if (this->current_entry + 1 == this->first_on_screen)
		{
		    this->first_on_screen =
			max(0, this->first_on_screen - this->scroll_step);
		    need_update_all = 1;
		}
		else
		{
		    if (!need_update)
			panel_update_entry(this, this->current_entry + 1);

		    need_update = 1;
		}
	    }

	    if (need_update_all)
		panel_update_entries(this);
	    else
		if (need_update)
		    panel_update_entry(this, this->current_entry);

	    panel_update_info(this);
	    break;

	case act_DOWN:
	    need_update_all = need_update = 0;

	    while (repeat_count--)
	    {
		if (this->current_entry < this->entries - 1)
		    this->current_entry++;
		else
		    break;

		if (this->current_entry - this->first_on_screen >=
		    this->lines - 2)
		{
		    this->first_on_screen = min(this->first_on_screen +
						this->scroll_step,
						this->entries - 1 -
						(this->lines - 2) + 1);
		    need_update_all = 1;
		    continue;
		}

		if (!need_update)
		    panel_update_entry(this, this->current_entry - 1);

		need_update = 1;
	    }

	    if (need_update_all)
		panel_update_entries(this);
	    else
		if (need_update)
		    panel_update_entry(this, this->current_entry);

	    panel_update_info(this);
	    break;

	case act_PGUP:
	    if (this->current_entry == 0)
		break;

	    old_current_entry = this->current_entry;

	    if (this->current_entry < this->lines - 2)
		panel_set_current_entry(this, 0);
	    else
		panel_set_current_entry(this,
					this->current_entry-(this->lines-2));

	    if (this->entries > this->lines - 2)
		panel_update_entries(this);
	    else
	    {
		panel_update_entry(this, old_current_entry);
		panel_update_entry(this, this->current_entry);
	    }

	    panel_update_info(this);
	    break;

	case act_PGDOWN:
	    if (this->current_entry == this->entries - 1)
		break;

	    old_current_entry = this->current_entry;

	    if (this->entries - 1 - this->first_on_screen < this->lines - 2)
		panel_set_current_entry(this, this->entries - 1);
	    else
		if (this->entries - 1 - this->current_entry < this->lines - 2)
		    panel_set_current_entry(this, this->entries - 1);
		else
		    panel_set_current_entry(this,
					    this->current_entry+this->lines-2);

	    if (this->entries > this->lines - 2)
		panel_update_entries(this);
	    else
	    {
		panel_update_entry(this, old_current_entry);
		panel_update_entry(this, this->current_entry);
	    }

	    panel_update_info(this);
	    break;

	case act_UP_ONE_DIR:
	    /* Go up one directory only if not already in `/'.  */
	    if (rootdir())
		break;

	    if (this->current_entry != 0)
		panel_set_current_entry(this, 0);

	    return panel_act_ENTER(this, other);

	case act_HOME:
	    if (this->current_entry != 0)
	    {
		panel_set_current_entry(this, 0);
		panel_update_entries(this);
		panel_update_info(this);
	    }

	    break;

	case act_END:
	    if (this->current_entry != this->entries - 1)
	    {
		panel_set_current_entry(this, this->entries - 1);
		panel_update_entries(this);
		panel_update_info(this);
	    }

	    break;

	case act_CHDIR:
	    panel_act_CHDIR(this, other, (char *)aux_info);
	    break;

	case act_ENABLE_NEXT_MODE:
	    this->display_mode = (this->display_mode + 1) % FILE_DISPLAY_MODES;
	    goto all_display_modes;

	case act_ENABLE_OWNER_GROUP:
	case act_ENABLE_DATE_TIME:
	case act_ENABLE_SIZE:
	case act_ENABLE_MODE:
	case act_ENABLE_FULL_NAME:
	case act_ENABLE_ALL:
	    this->display_mode = action - act_ENABLE_OWNER_GROUP;

	  all_display_modes:
	    /* Avoid displaying the ENABLE_ALL mode when the number of
	       columns is not big enough (we are in two panel mode).  */
	    if (this->columns < 80 && this->display_mode == ENABLE_ALL)
		this->display_mode = ENABLE_OWNER_GROUP;

	    panel_update_entries(this);
	    break;

	case act_SORT_NEXT_METHOD:
	    this->sort_method = (this->sort_method + 1) % FILE_SORT_METHODS;
	    goto all_sort_methodes;

	case act_SORT_BY_NAME:
	case act_SORT_BY_EXTENSION:
	case act_SORT_BY_SIZE:
	case act_SORT_BY_DATE:
	case act_SORT_BY_MODE:
	case act_SORT_BY_OWNER_ID:
	case act_SORT_BY_GROUP_ID:
	case act_SORT_BY_OWNER_NAME:
	case act_SORT_BY_GROUP_NAME:
	    this->sort_method = action - act_SORT_BY_NAME;

	  all_sort_methodes:
	    CurrentSortMethod = this->sort_method;
	    previous = this->dir_entry[this->current_entry].name;

	    /* Check if this is the root directory and sort without the
	       ".." entry if it is.  */
	    if (rootdir())
		qsort(this->dir_entry, this->entries,
		      sizeof(dir_entry_t), sort_compare_fn);
	    else
		qsort(this->dir_entry + 1, this->entries - 1,
		      sizeof(dir_entry_t), sort_compare_fn);

	    for (entry = 0; entry < this->entries; entry++)
		if (previous == this->dir_entry[entry].name)
		    break;

	    panel_set_current_entry(this, entry);
	    panel_update_entries(this);
	    break;

	case act_SWITCH:
	    xchg(&this->lines,   &other->lines);
	    xchg(&this->columns, &other->columns);
	    xchg(&this->x,       &other->x);
	    xchg(&this->y,       &other->y);

	    panel_resize(this, this->x, this->y,
			 this->lines, this->columns);
	    panel_resize(other, other->x, other->y,
			 other->lines, other->columns);
	    break;

	case act_PATTERN_SELECT:
	case act_PATTERN_UNSELECT:
	    /* Create a list with all the shell patterns.  */
	    safe_patterns = panel_parse_patterns((char *)aux_info);

	    for (patterns = safe_patterns; *patterns; patterns++)
		for (entry = 0; entry < this->entries; entry++)
		    if (this->dir_entry[entry].type != DIR_ENTRY)
		    {
			int fnm_flags = FNM_PATHNAME;

			if (LeadingDotMatch == OFF)
			    fnm_flags |= FNM_PERIOD;

			if (fnmatch(*patterns, this->dir_entry[entry].name,
				    fnm_flags) == 0)
			{
			    if (action == act_PATTERN_SELECT)
			    {
				if (!this->dir_entry[entry].selected)
				{
				    this->dir_entry[entry].selected = 1;
				    this->selected_entries++;
				}
			    }
			    else
			    {
				if (this->dir_entry[entry].selected)
				{
				    this->dir_entry[entry].selected = 0;
				    this->selected_entries--;
				}
			    }
			}
		    }

	    /* Deallocate the list of shell patterns.  */
	    panel_deallocate_patterns(safe_patterns);

	    panel_update_entries(this);
	    panel_update_info(this);
	    break;

	case act_REGET:
	    panel_act_REGET(this, aux_info);
	    break;

	case act_SET_SCROLL_STEP:
	    new_scroll_step = atoi((char *)aux_info);

	    if (new_scroll_step > 0 && new_scroll_step < this->lines - 1)
		this->scroll_step = other->scroll_step = new_scroll_step;

	    break;

	case act_ISEARCH_BEGIN:
	    this->isearch_stack = xstack_init(sizeof(isearch_t));
	    STACK_PUSH(this->current_entry, 0);
	    this->isearch_length  = 0;
	    this->wrapped_isearch = 0;
	    break;

	case act_ISEARCH_BACKWARD:
	    iai = (isearch_aux_t *)aux_info;
	    len = strlen(iai->string);

	    switch (iai->action)
	    {
		case IL_ISEARCH_ACTION_NONE:
		    break;

		case IL_ISEARCH_ACTION_DECREASE:
		    goto isearch_action_decrease;

		case IL_ISEARCH_ACTION_RETRY:
		    if (!this->wrapped_isearch)
			STACK_PUSH(this->current_entry, len);

		    /* Search backward.  */
		    result = panel_isearch_backward(this, iai->string, len,
						    this->wrapped_isearch ?
						    this->entries - 1 :
						    this->current_entry - 1);
		    goto isearch_backward_action_increase;

		case IL_ISEARCH_ACTION_INCREASE:
		    STACK_PUSH(this->current_entry, len);

		    /* Search backward.  */
		    result = panel_isearch_backward(this, iai->string, len,
						    this->current_entry);

		  isearch_backward_action_increase:
		    if (result == -1)
		    {
			iai->action = IL_ISEARCH_ACTION_FAILED;
			break;
		    }
		    else
			this->isearch_length = len;

		    if (this->wrapped_isearch)
		    {
			panel_set_current_entry(this, result);
			panel_update_entries(this);
			this->wrapped_isearch = 0;
		    }
		    else
			panel_action(this, act_UP, other, (void *)NULL,
				     this->current_entry - result);

		    break;

		default:
		    break;
	    }

	    iai->length = this->isearch_length;
	    break;

	case act_ISEARCH_FORWARD:
	    iai = (isearch_aux_t *)aux_info;
	    len = strlen(iai->string);

	    switch (iai->action)
	    {
		case IL_ISEARCH_ACTION_NONE:
		    break;

		case IL_ISEARCH_ACTION_DECREASE:
		  isearch_action_decrease:
		    {
			int prev_entry;
			size_t prev_length;

			/* Undo the last step of isearch-forward.  */
			STACK_POP(prev_entry, prev_length);

			if (this->isearch_length != len)
			    break;

			if (this->current_entry < prev_entry)
				panel_action(this, act_DOWN, other,
					     (void *)NULL,
					     prev_entry - this->current_entry);
			else
			    if (this->current_entry > prev_entry)
				panel_action(this, act_UP, other, (void *)NULL,
					     this->current_entry - prev_entry);

			STACK_PREVIEW(prev_entry, prev_length);

			this->isearch_length = prev_length;
		    }
		    break;

		case IL_ISEARCH_ACTION_RETRY:
		    if (!this->wrapped_isearch)
			STACK_PUSH(this->current_entry, len);

		    /* Search forward.  */
		    result = panel_isearch_forward(this, iai->string, len,
						   this->wrapped_isearch ?
						   0 :
						   this->current_entry + 1);
		    goto isearch_forward_action_increase;

		case IL_ISEARCH_ACTION_INCREASE:
		    STACK_PUSH(this->current_entry, len);

		    /* Search forward.  */
		    result = panel_isearch_forward(this, iai->string, len,
						   this->current_entry);

		  isearch_forward_action_increase:
		    if (result == -1)
		    {
			iai->action = IL_ISEARCH_ACTION_FAILED;
			break;
		    }
		    else
			this->isearch_length = len;

		    if (this->wrapped_isearch)
		    {
			panel_set_current_entry(this, result);
			panel_update_entries(this);
			this->wrapped_isearch = 0;
		    }
		    else
			panel_action(this, act_DOWN, other, (void *)NULL,
				     result - this->current_entry);
		    break;

		default:
		    break;
	    }

	    iai->length = this->isearch_length;
	    break;

	case act_ISEARCH_END:
	    xstack_end(this->isearch_stack);
	    this->isearch_stack = NULL;
	    break;

	case act_CMPDIR:
	    panel_act_CMPDIR(this, other, *(int *)aux_info);
	    break;

	case act_CASE:
	    panel_act_CASE(this, other, *(int *)aux_info);
	    break;

	case act_COMPARE:
	    panel_act_COMPARE(this, other);
	    break;

	case act_BIN_PACKING:
	    panel_act_BIN_PACKING(
                this, other, (off64_t)atoi((char *)aux_info));
	    break;

	case act_HORIZONTAL_SCROLL_LEFT:
	    this->horizontal_offset -= repeat_count;

	    if (this->horizontal_offset < 0)
		this->horizontal_offset = 0;

	    panel_update(this);
	    break;

	case act_HORIZONTAL_SCROLL_RIGHT:
	    this->horizontal_offset += repeat_count;

	    /* The +1 accounts for the type character (/, *, |, @, etc).  */
	    max_offset = (this->maxname + 1) -
		(this->columns - panel_get_reserved_characters(this));
	    if (max_offset < 0)
		max_offset = 0;

	    if (this->horizontal_offset >= max_offset)
		this->horizontal_offset = max_offset;

	    panel_update(this);
	    break;

	case act_SELECT_EXTENSION:
	case act_UNSELECT_EXTENSION:
	    ptr = strrchr(this->dir_entry[this->current_entry].name, '.');

	    /* If there is no dot in the name, or the dot is on the
	       first position, quit.  */
	    if (ptr == this->dir_entry[this->current_entry].name ||
		ptr == NULL)
		break;

	    extension = (char *)malloc(1 + strlen(ptr) + 1);
	    sprintf(extension, "*%s", ptr);

	    /* Select all the files that have the same extension as
	       the current one.  */
	    for (entry = 0; entry < this->entries; entry++)
		if (this->dir_entry[entry].type != DIR_ENTRY)
		{
		    int fnm_flags = FNM_PATHNAME;

		    if (LeadingDotMatch == OFF)
			fnm_flags |= FNM_PERIOD;

		    if (fnmatch(extension, this->dir_entry[entry].name,
				fnm_flags) == 0)
		    {
			if (action == act_SELECT_EXTENSION)
			{
			    if (!this->dir_entry[entry].selected)
			    {
				this->dir_entry[entry].selected = 1;
				this->selected_entries++;
			    }
			}
			else
			{
			    if (this->dir_entry[entry].selected)
			    {
				this->dir_entry[entry].selected = 0;
				this->selected_entries--;
			    }
			}
		    }
		}

	    panel_update_entries(this);
	    panel_update_info(this);
	    break;

	default:
	    fatal("no action");
    }

    return 0;
}
