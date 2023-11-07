/* panel.h -- The data structures and function prototypes used in panel.c.  */

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

#ifndef _GIT_PANEL_H
#define _GIT_PANEL_H


#include <sys/types.h>
#include <limits.h>

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif


#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_DIRENT_H
#include <dirent.h>
#define NAMLEN(dirent) strlen((dirent)->d_name)
#else /* !HAVE_DIRENT_H */
#define dirent direct
#define NAMLEN(dirent) (dirent)->d_namlen
#ifdef HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif /* HAVE_SYS_NDIR_H */
#ifdef HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif /* HAVE_SYS_DIR_H */
#ifdef HAVE_NDIR_H
#include <ndir.h>
#endif /* HAVE_NDIR_H */
#endif /* !HAVE_DIRENT_H */

#include "stdc.h"
#include "xstack.h"
#include "file.h"

#define IS_SPECIAL(x) ( ! (S_ISLNK((x)) || S_ISDIR((x)) || S_ISREG((x))))

/* Panel actions.  */
#define act_NOACTION			 0
#define act_ENTER			 1
#define act_COPY			 2
#define act_DELETE			 3
#define act_SELECT			 4
#define act_SELECT_ALL			 5
#define act_UNSELECT_ALL		 6
#define act_TOGGLE			 7

#define act_ENABLE_NEXT_MODE		 8
#define act_ENABLE_OWNER_GROUP		 9
#define act_ENABLE_DATE_TIME		10
#define act_ENABLE_SIZE			11
#define act_ENABLE_MODE			12
#define act_ENABLE_FULL_NAME		13
#define act_ENABLE_ALL			14

#define act_SORT_NEXT_METHOD		15
#define act_SORT_BY_NAME		16
#define act_SORT_BY_EXTENSION		17
#define act_SORT_BY_SIZE		18
#define act_SORT_BY_DATE		19
#define act_SORT_BY_MODE		20
#define act_SORT_BY_OWNER_ID		21
#define act_SORT_BY_GROUP_ID		22
#define act_SORT_BY_OWNER_NAME		23
#define act_SORT_BY_GROUP_NAME		24

#define act_MKDIR			25
#define act_MOVE			26
#define act_UP				27
#define act_DOWN			28
#define act_PGUP			29
#define act_PGDOWN			30
#define act_HOME			31
#define act_END				32
#define act_CHDIR			33
#define act_REGET			34
#define act_SWITCH			35

#define act_PATTERN_SELECT		36
#define act_PATTERN_UNSELECT		37

#define act_SET_SCROLL_STEP		38

#define act_ISEARCH_BEGIN		39
#define act_ISEARCH_BACKWARD		40
#define act_ISEARCH_FORWARD		41
#define act_ISEARCH_END			42

#define act_CMPDIR			43
#define act_CASE			44
#define act_UP_ONE_DIR			45
#define act_COMPARE			46
#define act_BIN_PACKING			47

#define act_HORIZONTAL_SCROLL_LEFT	48
#define act_HORIZONTAL_SCROLL_RIGHT	49

#define act_SELECT_EXTENSION		50
#define act_UNSELECT_EXTENSION		51

/* File sort methods.  */
#define SORT_BY_NAME		0
#define SORT_BY_EXTENSION	1
#define SORT_BY_SIZE		2
#define SORT_BY_DATE		3
#define SORT_BY_MODE		4
#define SORT_BY_OWNER_ID	5
#define SORT_BY_GROUP_ID	6
#define SORT_BY_OWNER_NAME	7
#define SORT_BY_GROUP_NAME	8


/* File display modes.  */
#define ENABLE_OWNER_GROUP	0
#define ENABLE_DATE_TIME	1
#define ENABLE_SIZE		2
#define ENABLE_ABBREVSIZE	3
#define ENABLE_MODE		4
#define ENABLE_FULL_NAME	5
#define ENABLE_ALL		6


/* File types.  */
#define DIR_ENTRY		0
#define FILE_ENTRY		1
#define SYMLINK_ENTRY		2
#define FIFO_ENTRY		3
#define SOCKET_ENTRY		4


/* Current file marker types.  */
#define ACTIVE_PANEL_MARKER	'>'
#define INACTIVE_PANEL_MARKER	'*'


/* Comparison types.  */
#define CMPDIR_THOROUGH		0
#define CMPDIR_QUICK		1


/* Case conversion types.  */
#define CASE_DOWN		0
#define CASE_UP			1


/* Internal structure, used by the push/pop functions to keep track of
   isearched entries.  */
typedef struct
{
    int entry;		/* the panel entry number.  */
    size_t length;	/* isearch_length at match time.  See below.  */
} isearch_t;


/* Structure used to pass isearch-specific parameters to panel_action.  */
typedef struct
{
    int action;		/* the action to be performed.  */
    char *string;	/* the current isearched string.  */
    size_t length;	/* the new length of the isearched string.  */
} isearch_aux_t;


typedef struct
{
    char *name;
    off64_t size;
    mode_t mode;
    uid_t uid;
    gid_t gid;
    time_t mtime;
    char *owner;
    char *group;
    char date[16];
    char type;
    char selected;
    char executable;
    char fti_loaded;
    char foreground;
    char background;
    char brightness;
} dir_entry_t;


typedef struct
{
    DIR *dir;			/* the current directory.  */
    window_t *window;		/* the panel window.  */
#ifdef HAVE_LINUX
    int msdosfs;		/* the current panel directory resides
				   on a MSDOG direrctory.  Argh...  */
#endif  /* HAVE_LINUX */
    int on_screen;		/* # of files on the screen.  */
    char *path;			/* The current path.  */
    char *temp;			/* A temporary string.  */
    int current_entry;		/* The current file entry #.  */
    int first_on_screen;	/* The # of the entry displayed in the
				   panel's first line.  */
    int maxname;		/* Don't bother :-) */
    dir_entry_t *dir_entry;	/* A record for each file in the
				   current directory.  */
    size_t isearch_length;	/* The current length of the string
				   used in incremental search.  */
    xstack_t *isearch_stack;	/* The isearch stack.  */
    int last_index;		/* Used in the iterator.  */
    int multiple_files;		/* A flag used in the iterator.  */
    int x, y;			/* The panel's position.  */
    int lines, columns;		/* The panel is that big.  */
    int entries;		/* The # of files/directories.  */
    char focus;			/* The panel's focus flag.  */
    char visible;		/* The panel's visible flag.  */
    char wrapped_isearch;	/* The wrapped isearch_flag.  */
    int selected_entries;	/* The # of selected entries.  */
    int horizontal_offset;	/* The horizontal scroll offset.  */
    size_t pathlen;		/* The path length.  */
    int display_mode;		/* The current display mode.  */
    int sort_method;		/* The current sort method.  */
    int scroll_step;		/* The scroll step.  */
    int thumb;			/* The position of the thumb.  */
    int chkdest;		/* Don't bother :-).  */
} panel_t;


extern panel_t *panel_init PROTO ((char *));
extern void panel_end PROTO ((panel_t *));
extern void panel_init_iterator PROTO ((panel_t *));
extern void panel_select_all PROTO ((panel_t *));
extern void panel_unselect_all PROTO ((panel_t *));
extern int panel_get_next PROTO ((panel_t *));
extern void panel_set_focus PROTO ((panel_t *, int));
extern void panel_no_optimizations PROTO ((panel_t *));
extern int panel_action PROTO ((panel_t *, int, panel_t *, void *, int));
extern window_t *panel_window PROTO ((panel_t *));
extern char *panel_get_path PROTO ((panel_t *));
extern char *panel_get_current_file_name PROTO ((panel_t *));
extern uid_t panel_get_current_file_uid PROTO ((panel_t *));
extern gid_t panel_get_current_file_gid PROTO ((panel_t *));
extern mode_t panel_get_current_file_mode PROTO ((panel_t *));
extern int panel_get_current_file_type	PROTO ((panel_t *));
extern void panel_resize PROTO ((panel_t *, size_t, size_t, size_t, size_t));
extern void panel_activate PROTO ((panel_t *));
extern void panel_deactivate PROTO ((panel_t *));
extern void panel_update PROTO ((panel_t *));
extern void panel_set_wrapped_isearch_flag PROTO ((panel_t *, int));
extern void panel_center_current_entry PROTO ((panel_t *));


#endif  /* _GIT_PANEL_H */
