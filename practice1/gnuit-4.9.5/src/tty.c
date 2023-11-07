/* tty.c -- The tty management file.  */

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

#ifdef HAVE_STDDEF_H
#include <stddef.h>
#endif

#include <ctype.h>
#include "file.h"
#include <sys/ioctl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#include <assert.h>
#include "stdc.h"
#include "xstring.h"
#include "xmalloc.h"
#include "xio.h"
#include "tty.h"
#include "signals.h"
#include "misc.h"


/* Stolen from GNU Emacs.  */
#ifdef _POSIX_VDISABLE
#define CDISABLE _POSIX_VDISABLE
#else /* not _POSIX_VDISABLE */
#ifdef CDEL
#undef CDISABLE
#define CDISABLE CDEL
#else /* not CDEL */
#define CDISABLE 255
#endif /* not CDEL */
#endif /* not _POSIX_VDISABLE */

#define MAX_TTY_COLUMNS         1024
#define MAX_TTY_LINES           1024


/* I want to avoid including curses.h or any other header file that
   defines these.  I think it's safer because I couldn't find 2
   similar curses.h files in the entire world...  */

extern int tputs PROTO ((const char *__string, int __nlines, int (*outfun)()));
extern int tgetent PROTO ((void *__buffer, const char *__termtype));
extern char *tgetstr PROTO ((const char *__name, char **__area));
extern int tgetnum PROTO ((const char *__name));
extern int tgetflag PROTO ((const char *__name));
extern char *tgoto PROTO ((const char *__cstring, int __hpos, int __vpos));


#define TTY_INPUT       0
#define TTY_OUTPUT      1

#ifdef HAVE_LINUX
#define VCS_READ	1
#define VCS_WRITE	2
#endif

#ifdef HAVE_LINUX
static int vcs_read_ok;
static int vcs_is_monochrome;
#endif

/* If tty_kbdmode == TTY_FULL_INPUT, single character key sequences
   are inserted into the linked list.  This feature is used by gitps
   which has no command line.  */
static int tty_kbdmode;

#ifdef HAVE_POSIX_TTY
static struct termios old_term;
static struct termios new_term;
#else
#ifdef HAVE_SYSTEMV_TTY
static struct termio old_term;
static struct termio new_term;
#else
static struct sgttyb  old_arg;
static struct tchars  old_targ;
static struct ltchars old_ltarg;
static struct sgttyb  new_arg;
static struct tchars  new_targ;
static struct ltchars new_ltarg;

/* NextStep doesn't define TILDE.  */
#ifndef TILDE
#define TILDE 0
#endif

#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

int tty_lines;
int tty_columns;
char *tty_device;

static unsigned char *tty_key_seq;

static int tty_device_length;
static int tty_last_char_flag;
static int tty_cursor_x;
static int tty_cursor_y;

/*
 * tty_*_current_attribute:
 *   bit 7:		reverse video
 *   bit 6:		brightness
 *   bits 5,4,3:	background color
 *   bits 2,1,0:	foreground color
 */
static unsigned char tty_current_attribute;
static unsigned char tty_io_current_attribute;

/* The current interrupt character.  We need to restore when resuming
   after a suspend.  */
static int tty_interrupt_char = key_INTERRUPT;

#define INVALID_CACHE 0
#define VALID_CACHE   1

static int fg_cache = INVALID_CACHE;
static int bg_cache = INVALID_CACHE;
static int br_cache = INVALID_CACHE;
static int rv_cache = INVALID_CACHE;

/* tty_scr the current status of the screen, while tty_atr is used to
   keep the current status of the attributes.  */
static unsigned char *tty_scr;
static unsigned char *tty_atr;

/* tty_prev_scr will always contain the copy of the previous screen,
   while tty_atr will contain the copy of the previous attributes.  */
static unsigned char *tty_prev_scr;
static unsigned char *tty_prev_atr;


/* The ANSI color sequences.  */
static char ansi_foreground[] = { 0x1b, '[', '3', '0', 'm' };
static char ansi_background[] = { 0x1b, '[', '4', '0', 'm' };
static char ansi_defaults[]   = { 0x1b, '[', '0', 'm' };

/* These variable tells us if we should use standard ANSI color sequences.
   Its value is taken from the configuration file.  */
extern int AnsiColors;

#define FOREGROUND_MASK 0x07
#define BACKGROUND_MASK 0x38
#define BRIGHTNESS_MASK 0x40
#define REVERSEVID_MASK 0x80


#define _TTY_FOREGROUND(attributes) ((((int)attributes)&FOREGROUND_MASK) >> 0)
#define _TTY_BACKGROUND(attributes) ((((int)attributes)&BACKGROUND_MASK) >> 3)
#define _TTY_BRIGHTNESS(attributes) ((((int)attributes)&BRIGHTNESS_MASK) >> 6)
#define _TTY_REVERSEVID(attributes) ((((int)attributes)&REVERSEVID_MASK) >> 7)

#define TTY_IO_FOREGROUND _TTY_FOREGROUND(tty_io_current_attribute)
#define TTY_IO_BACKGROUND _TTY_BACKGROUND(tty_io_current_attribute)
#define TTY_IO_BRIGHTNESS _TTY_BRIGHTNESS(tty_io_current_attribute)
#define TTY_IO_REVERSEVID _TTY_REVERSEVID(tty_io_current_attribute)

#define TTY_FOREGROUND _TTY_FOREGROUND(tty_current_attribute)
#define TTY_BACKGROUND _TTY_BACKGROUND(tty_current_attribute)
#define TTY_BRIGHTNESS _TTY_BRIGHTNESS(tty_current_attribute)
#define TTY_REVERSEVID _TTY_REVERSEVID(tty_current_attribute)

#define _TTY_SET_FOREGROUND(attributes, color)\
    ((attributes) = ((attributes) & ~FOREGROUND_MASK) | (((color) & 7) << 0))
#define _TTY_SET_BACKGROUND(attributes, color)\
    ((attributes) = ((attributes) & ~BACKGROUND_MASK) | (((color) & 7) << 3))
#define _TTY_SET_BRIGHTNESS(attributes, status)\
    ((attributes) = ((attributes) & ~BRIGHTNESS_MASK) | (((status) & 1) << 6))
#define _TTY_SET_REVERSEVID(attributes, status)\
    ((attributes) = ((attributes) & ~REVERSEVID_MASK) | (((status) & 1) << 7))

#define TTY_IO_SET_FOREGROUND(color)\
    _TTY_SET_FOREGROUND(tty_io_current_attribute, (color))
#define TTY_IO_SET_BACKGROUND(color)\
    _TTY_SET_BACKGROUND(tty_io_current_attribute, (color))
#define TTY_IO_SET_BRIGHTNESS(status)\
    _TTY_SET_BRIGHTNESS(tty_io_current_attribute, (status))
#define TTY_IO_SET_REVERSEVID(status)\
    _TTY_SET_REVERSEVID(tty_io_current_attribute, (status))

#define TTY_SET_FOREGROUND(color)\
    _TTY_SET_FOREGROUND(tty_current_attribute, (color))
#define TTY_SET_BACKGROUND(color)\
    _TTY_SET_BACKGROUND(tty_current_attribute, (color))
#define TTY_SET_BRIGHTNESS(status)\
    _TTY_SET_BRIGHTNESS(tty_current_attribute, (status))
#define TTY_SET_REVERSEVID(status)\
    _TTY_SET_REVERSEVID(tty_current_attribute, (status))


#ifdef HAVE_LINUX
/* These variable tells us if we are using a Linux console.  */
int LinuxConsole;
#endif /* HAVE_LINUX */


/* Structures for keys management.  */
tty_key_t *key_list_head;
tty_key_t *current_key;
tty_key_t default_key;

/* 1Kb of cache memory for terminal optimizations.  Don't make this
   bigger, some terminal/OS combinations fail to transmit chunks of
   data that are too big.  I'm not sure I fully understand the problem,
   but 10Kb didn't work and 1Kb did.  xwrite() reported success,
   but the screen was not correctly updated.  xterm bug!?  */
#define TTY_CACHE_SIZE 1024

static char tty_cache[TTY_CACHE_SIZE];
static int tty_index;

#ifndef HAVE_LINUX
static char term_buf[2048];
#endif
static char vt100[] = "vt100";

/* The terminal mode. TTY_CANONIC at the begining.  */
int tty_mode = TTY_CANONIC;

char *tty_type;

char PC;        /* For tputs.  */
char *BC;       /* For tgoto/tparm.  */
char *UP;

#ifdef HAVE_LINUX
speed_t ospeed;
#else /* !HAVE_LINUX */
short ospeed;
#endif /* !HAVE_LINUX */


/* A structure describing some attributes we need to know about each
   capability. See below for greater detail.  */
typedef struct
{
    char *name;		/* the capability name.  */

    /* These ones should be in an union, but the HP-UX non ANSI compiler
       complains about union initialization being an ANSI feature and I
       care more for portability than for the memory used.  */
    char *string;	/* The string  value of the capability.  */
    int  integer;	/* The integer value of the capability.  */

    int required;	/* This capability is required.  */
    char *symbol;	/* The human readable form of the key name.  */
} tty_capability_t;


#define TTY_CAPABILITIES_USED   38
#define TTY_FIRST_SYMBOL_KEY    17

static tty_capability_t tty_capability[TTY_CAPABILITIES_USED] =
{
    { "me", NULL, 0, 0, NULL },		/* Turn off all attributes.  */
    { "mr", NULL, 0, 0, NULL },		/* Turn on reverse video mode.  */
    { "md", NULL, 0, 0, NULL },		/* Turn on bold.  */
    { "vi", NULL, 0, 0, NULL },		/* Make the cursor invisible.  */
    { "ve", NULL, 0, 0, NULL },		/* Make the cursor appear normal.  */
    { "cl", NULL, 0, 1, NULL },		/* Clear screen & home the cursor.  */
    { "cm", NULL, 0, 1, NULL },		/* Move the cursor.  */
    { "pc", NULL, 0, 0, NULL },		/* Padding character.  */
    { "up", NULL, 0, 0, NULL },		/* Up one line.  */
    { "le", NULL, 0, 0, NULL },		/* Move left one space.  */
    { "so", NULL, 0, 0, NULL },		/* Enter standout mode.  */
    { "sg", NULL, 0, 0, NULL },		/* This is a magic-cookie terminal.  */
    { "ms", NULL, 0, 0, NULL },		/* Safe to move in standout mode.  */
    { "co", NULL, 0, 0, NULL },		/* The number of columns.  */
    { "li", NULL, 0, 0, NULL },		/* The number of lines.  */
    { "ti", NULL, 0, 0, NULL },		/* Begin program that uses cursor motion.  */
    { "te", NULL, 0, 0, NULL },		/* End program that uses cursor motion.  */
    { "ku", NULL, 0, 0, "UP" },		/* (UP) */
    { "kd", NULL, 0, 0, "DOWN" },	/* (DOWN) */
    { "kr", NULL, 0, 0, "RIGHT" },	/* (RIGHT) */
    { "kl", NULL, 0, 0, "LEFT" },	/* (LEFT) */
    { "kI", NULL, 0, 0, "INS" },	/* (INS) */
    { "kD", NULL, 0, 0, "DEL" },	/* (DEL) */
    { "kh", NULL, 0, 0, "HOME" },	/* (HOME) */
    { "@7", NULL, 0, 0, "END" },	/* (END) */
    { "kP", NULL, 0, 0, "PGUP" },	/* (PGUP) */
    { "kN", NULL, 0, 0, "PGDOWN" },	/* (PGDOWN) */
    { "k0", NULL, 0, 0, "F0" },		/* (F0) */
    { "k1", NULL, 0, 0, "F1" },		/* (F1) */
    { "k2", NULL, 0, 0, "F2" },		/* (F2) */
    { "k3", NULL, 0, 0, "F3" },		/* (F3) */
    { "k4", NULL, 0, 0, "F4" },		/* (F4) */
    { "k5", NULL, 0, 0, "F5" },		/* (F5) */
    { "k6", NULL, 0, 0, "F6" },		/* (F6) */
    { "k7", NULL, 0, 0, "F7" },		/* (F7) */
    { "k8", NULL, 0, 0, "F8" },		/* (F8) */
    { "k9", NULL, 0, 0, "F9" },		/* (F9) */
    { "k;", NULL, 0, 0, "F10" },	/* (F10) */
};


/* Some nice aliases...  */
#define TTY_ATTRIBUTES_OFF      (tty_capability[0].string)
#define TTY_REVERSE_ON          (tty_capability[1].string)
#define TTY_BRIGHT_ON           (tty_capability[2].string)
#define TTY_CURSOR_OFF          (tty_capability[3].string)
#define TTY_CURSOR_ON           (tty_capability[4].string)
#define TTY_CLEAR_SCREEN        (tty_capability[5].string)
#define TTY_CURSOR_MOVE         (tty_capability[6].string)
#define TTY_PAD_CHAR            (tty_capability[7].string)
#define TTY_UP_ONE_LINE         (tty_capability[8].string)
#define TTY_LEFT_ONE_SPACE      (tty_capability[9].string)
#define TTY_STANDOUT_ON		(tty_capability[10].string)
#define TTY_MAGIC_COOKIE	(tty_capability[11].integer)
#define TTY_MS_FLAG		(tty_capability[12].integer)
#define TTY_COLUMNS		(tty_capability[13].integer)
#define TTY_LINES		(tty_capability[14].integer)
#define TTY_START_CURSORAPP	(tty_capability[15].string)
#define TTY_END_CURSORAPP	(tty_capability[16].string)

/* Some more nice aliases...  */
#define TTY_ATTRIBUTES_OFF_NAME 	(tty_capability[0].name)
#define TTY_REVERSE_ON_NAME     	(tty_capability[1].name)
#define TTY_BRIGHT_ON_NAME      	(tty_capability[2].name)
#define TTY_CURSOR_OFF_NAME     	(tty_capability[3].name)
#define TTY_CURSOR_ON_NAME      	(tty_capability[4].name)
#define TTY_CLEAR_SCREEN_NAME   	(tty_capability[5].name)
#define TTY_CURSOR_MOVE_NAME    	(tty_capability[6].name)
#define TTY_PAD_CHAR_NAME       	(tty_capability[7].name)
#define TTY_UP_ONE_LINE_NAME    	(tty_capability[8].name)
#define TTY_LEFT_ONE_SPACE_NAME 	(tty_capability[9].name)
#define TTY_STANDOUT_ON_NAME		(tty_capability[10].name)
#define TTY_MAGIC_COOKIE_NAME		(tty_capability[11].name)
#define TTY_MS_FLAG_NAME		(tty_capability[12].name)
#define TTY_COLUMNS_NAME		(tty_capability[13].name)
#define TTY_LINES_NAME			(tty_capability[14].name)
#define TTY_START_CURSORAPP_NAME	(tty_capability[15].name)
#define TTY_END_CURSORAPP_NAME		(tty_capability[16].name)


#ifdef HAVE_LIBTERMCAP

static char term_database[] = "termcap";
static char term_env[]      = "TERMCAP";

#else   /* !HAVE_LIBTERMCAP */

static char term_database[] = "terminfo";
static char term_env[]      = "TERMINFO";

#endif  /* !HAVE_LIBTERMCAP */


static void tty_io_goto PROTO ((int, int));
static void tty_io_foreground PROTO ((int));
static void tty_io_background PROTO ((int));
static void tty_io_brightness PROTO ((int));
static void tty_io_reversevid PROTO ((int));
static void tty_io_colors PROTO ((int));
static int  tty_is_xterm PROTO ((char *));


extern void fatal PROTO ((char *));


/* ANSI colors. OFF & ON are here because we need them when we read the
   configuration file. Don't bother...  */
static char *colors[10] =
{
    "BLACK",
    "RED",
    "GREEN",
    "YELLOW",
    "BLUE",
    "MAGENTA",
    "CYAN",
    "WHITE",
    "OFF",
    "ON"
};


/* Control keys description. C-a, C-b, C-c and so on...  */
unsigned char key_ctrl_table[0x5f] =
{
    0x20,                       /* 0x20 ( ) */
    0x21,                       /* 0x21 (!) */
    0x22,                       /* 0x22 (") */
    0x23,                       /* 0x23 (#) */
    0xff,                       /* 0x24 ($) */
    0x25,                       /* 0x25 (%) */
    0x26,                       /* 0x26 (&) */
    0x07,                       /* 0x27 (') */
    0x28,                       /* 0x28 (() */
    0x29,                       /* 0x29 ()) */
    0x2a,                       /* 0x2a (*) */
    0x2b,                       /* 0x2b (+) */
    0x2c,                       /* 0x2c (,) */
    0x2d,                       /* 0x2d (-) */
    0x2e,                       /* 0x2e (.) */
    0x2f,                       /* 0x2f (/) */
    0x20,                       /* 0x30 (0) */
    0x20,                       /* 0x31 (1) */
    0xff,                       /* 0x32 (2) */
    0x1b,                       /* 0x33 (3) */
    0x1c,                       /* 0x34 (4) */
    0x1d,                       /* 0x35 (5) */
    0x1e,                       /* 0x36 (6) */
    0x1f,                       /* 0x37 (7) */
    0x7f,                       /* 0x38 (8) */
    0x39,                       /* 0x39 (9) */
    0x3a,                       /* 0x3a (:) */
    0x3b,                       /* 0x3b (;) */
    0x3c,                       /* 0x3c (<) */
    0x20,                       /* 0x3d (=) */
    0x3e,                       /* 0x3e (>) */
    0x20,                       /* 0x3f (?) */
    0x20,                       /* 0x40 (@) */
    0x01,                       /* 0x41 (A) */
    0x02,                       /* 0x42 (B) */
    0x03,                       /* 0x43 (C) */
    0x04,                       /* 0x44 (D) */
    0x05,                       /* 0x45 (E) */
    0x06,                       /* 0x46 (F) */
    0x07,                       /* 0x47 (G) */
    0x08,                       /* 0x48 (H) */
    0x09,                       /* 0x49 (I) */
    0x0a,                       /* 0x4a (J) */
    0x0b,                       /* 0x4b (K) */
    0x0c,                       /* 0x4c (L) */
    0x0d,                       /* 0x4d (M) */
    0x0e,                       /* 0x4e (N) */
    0x0f,                       /* 0x4f (O) */
    0x10,                       /* 0x50 (P) */
    0x11,                       /* 0x51 (Q) */
    0x12,                       /* 0x52 (R) */
    0x13,                       /* 0x53 (S) */
    0x14,                       /* 0x54 (T) */
    0x15,                       /* 0x55 (U) */
    0x16,                       /* 0x56 (V) */
    0x17,                       /* 0x57 (W) */
    0x18,                       /* 0x58 (X) */
    0x19,                       /* 0x59 (Y) */
    0x1a,                       /* 0x5a (Z) */
    0x1b,                       /* 0x5b ([) */
    0x1c,                       /* 0x5c (\) */
    0x1d,                       /* 0x5d (]) */
    0x5e,                       /* 0x5e (^) */
    0x7f,                       /* 0x5f (_) */
    0x20,                       /* 0x60 (`) */
    0x01,                       /* 0x61 (a) */
    0x02,                       /* 0x62 (b) */
    0x03,                       /* 0x63 (c) */
    0x04,                       /* 0x64 (d) */
    0x05,                       /* 0x65 (e) */
    0x06,                       /* 0x66 (f) */
    0x07,                       /* 0x67 (g) */
    0x08,                       /* 0x68 (h) */
    0x09,                       /* 0x69 (i) */
    0x0a,                       /* 0x6a (j) */
    0x0b,                       /* 0x6b (k) */
    0x0c,                       /* 0x6c (l) */
    0x0d,                       /* 0x6d (m) */
    0x0e,                       /* 0x6e (n) */
    0x0f,                       /* 0x6f (o) */
    0x10,                       /* 0x70 (p) */
    0x11,                       /* 0x71 (q) */
    0x12,                       /* 0x72 (r) */
    0x13,                       /* 0x73 (s) */
    0x14,                       /* 0x74 (t) */
    0x15,                       /* 0x75 (u) */
    0x16,                       /* 0x76 (v) */
    0x17,                       /* 0x77 (w) */
    0x18,                       /* 0x78 (x) */
    0x19,                       /* 0x79 (y) */
    0x1a,                       /* 0x7a (z) */
    0x20,                       /* 0x7b ({) */
    0x20,                       /* 0x7c (|) */
    0x20,                       /* 0x7d (}) */
    0x20,                       /* 0x7e (~) */
};


#define NO      0
#define YES     1

#define MAX_KEY_LENGTH	15

/* Major number for Linux virtual console devices (/dev/tty*).  */
#define LINUX_VC_MAJOR  4


static int  keyno    = 0;
static int  keyindex = 0;
static char keybuf[1024];
static unsigned char keystr[MAX_KEY_LENGTH * 20];
static int partial = 0;
static int key_on_display = 0;

/* Hooks that get called when we are going idle and when we stop
   being idle.  */
void (* tty_enter_idle_hook)();
void (* tty_exit_idle_hook)();


void
tty_set_last_char_flag(last_char_flag)
    int last_char_flag;
{
    tty_last_char_flag = last_char_flag;
}


/*
 * This function is used to switch between canonical and noncanonical
 * terminal modes.
 */
void
tty_set_mode(mode)
    int mode;
{
    if (mode == TTY_NONCANONIC)
    {
#ifdef HAVE_POSIX_TTY
	new_term = old_term;
	new_term.c_iflag &= ~(IXON | ICRNL | IGNCR | INLCR | IGNBRK | BRKINT);
	new_term.c_oflag &= ~OPOST;
	new_term.c_lflag |= ISIG | NOFLSH;
	new_term.c_lflag &= ~(ICANON | ECHO);

	/* I think we will always have these ones:  */

	new_term.c_cc[VINTR] = key_INTERRUPT;		/* Ctrl-G */
	new_term.c_cc[VQUIT] = CDISABLE;

#ifdef VSTART
	new_term.c_cc[VSTART] = CDISABLE;		/* START (^Q) */
#endif

#ifdef VSTOP
	new_term.c_cc[VSTOP] = CDISABLE;		/* STOP (^S) */
#endif

	new_term.c_cc[VMIN] = 1;
	new_term.c_cc[VTIME] = 0;

	/* ... but not always these ones: (in fact I am not sure if I
	   really need to overwrite all these, but just in case... */

#ifdef VERASE
	new_term.c_cc[VERASE] = CDISABLE;
#endif

#ifdef VKILL
	new_term.c_cc[VKILL] = CDISABLE;
#endif

#ifdef VEOL
	new_term.c_cc[VEOL] = CDISABLE;
#endif

#ifdef VEOL2
	new_term.c_cc[VEOL2] = CDISABLE;
#endif

#ifdef VSWTCH
	new_term.c_cc[VSWTCH] = CDISABLE;
#endif

#ifdef VSUSP
	new_term.c_cc[VSUSP] = key_SUSPEND;             /* Ctrl-Z */
#endif

#ifdef VDSUSP
	new_term.c_cc[VDSUSP] = CDISABLE;
#endif

#ifdef VREPRINT
	new_term.c_cc[VREPRINT] = CDISABLE;
#endif

#ifdef VDISCARD
	new_term.c_cc[VDISCARD] = CDISABLE;
#endif

#ifdef VWERASE
	new_term.c_cc[VWERASE] = CDISABLE;
#endif

#ifdef VLNEXT
	new_term.c_cc[VLNEXT] = CDISABLE;
#endif

	tcsetattr(TTY_OUTPUT, TCSADRAIN, &new_term);
	ospeed = cfgetospeed(&new_term);
#else
#ifdef HAVE_SYSTEMV_TTY
	new_term = old_term;
	new_term.c_iflag &= ~(IXON | ICRNL | IGNCR | INLCR);
	new_term.c_oflag = 0;
	new_term.c_lflag = 0;

	/* I think we will always have these:  */

	new_term.c_cc[VINTR] = key_INTERRUPT;	/* Ctrl-G */
	new_term.c_cc[VQUIT] = CDISABLE;

#ifdef VSTART
	new_term.c_cc[VSTART] = CDISABLE;	/* START (^Q) */
#endif

#ifdef VSTOP
	new_term.c_cc[VSTOP] = CDISABLE;	/* STOP (^S) */
#endif

	new_term.c_cc[VMIN] = 1;
	new_term.c_cc[VTIME] = 0;

	/* ... but not always these:  (in fact I am not sure if I really
	   need to overwrite all these, but just in case... */

#ifdef VERASE
	new_term.c_cc[VERASE] = CDISABLE;
#endif

#ifdef VKILL
	new_term.c_cc[VKILL] = CDISABLE;
#endif

#ifdef VEOL
	new_term.c_cc[VEOL] = CDISABLE;
#endif

#ifdef VEOL2
	new_term.c_cc[VEOL2] = CDISABLE;
#endif

#ifdef VSWTCH
	new_term.c_cc[VSWTCH] = CDISABLE;
#endif

#ifdef VSUSP
	new_term.c_cc[VSUSP] = key_SUSPEND;             /* Ctrl-Z */
#endif

#ifdef VDSUSP
	new_term.c_cc[VDSUSP] = CDISABLE;
#endif

#ifdef VREPRINT
	new_term.c_cc[VREPRINT] = CDISABLE;
#endif

#ifdef VDISCARD
	new_term.c_cc[VDISCARD] = CDISABLE;
#endif

#ifdef VWERASE
	new_term.c_cc[VWERASE] = CDISABLE;
#endif

#ifdef VLNEXT
	new_term.c_cc[VLNEXT] = CDISABLE;
#endif

	ioctl(TTY_OUTPUT, TCSETAW, &new_term);
	ospeed = new_term.c_cflag & CBAUD;
#else
	new_arg   = old_arg;
	new_targ  = old_targ;
	new_ltarg = old_ltarg;
	new_arg.sg_flags = ((old_arg.sg_flags &
			 ~(ECHO | CRMOD | XTABS | ALLDELAY | TILDE)) | CBREAK);
	new_targ.t_intrc   = key_INTERRUPT;     /* Ctrl-G */
	new_targ.t_quitc   = CDISABLE;
	new_targ.t_stopc   = CDISABLE;
	new_targ.t_startc  = CDISABLE;
	new_targ.t_eofc    = CDISABLE;
	new_targ.t_brkc    = CDISABLE;
	new_ltarg.t_lnextc = CDISABLE;
	new_ltarg.t_flushc = CDISABLE;
	new_ltarg.t_werasc = CDISABLE;
	new_ltarg.t_rprntc = CDISABLE;
	new_ltarg.t_dsuspc = CDISABLE;   	/* DSUSPC (delayed SUSPC,^Y) */
	new_ltarg.t_suspc  = key_SUSPEND;	/* Ctrl-Z */

	ioctl(TTY_OUTPUT, TIOCSETN, &new_arg);
	ioctl(TTY_OUTPUT, TIOCSETC, &new_targ);
	ioctl(TTY_OUTPUT, TIOCSLTC, &new_ltarg);
	ospeed = new_arg.sg_ospeed;
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

/* Try to make sure the terminal is not locked.  */
#ifdef TCXONC
#ifdef __QNX__
	{
	    int value = 1;
	    ioctl (TTY_OUTPUT, TCXONC, &value);
	}
#else
	ioctl(TTY_OUTPUT, TCXONC, 1);
#endif
#endif

#ifndef APOLLO
#ifdef TIOCSTART
	ioctl(TTY_OUTPUT, TIOCSTART, 0);
#endif
#endif

#ifdef HAVE_POSIX_TTY
#ifdef TCOON
	tcflow(TTY_OUTPUT, TCOON);
#endif
#endif
	/* Make sure we restore the interrupt character that was in
	   use last time when we used NONCANONICAL mode.  */
	tty_set_interrupt_char(tty_interrupt_char);
    }
    else
    {
#ifdef HAVE_POSIX_TTY
	tcsetattr(TTY_OUTPUT, TCSADRAIN, &old_term);
#else
#ifdef HAVE_SYSTEMV_TTY
	ioctl(TTY_OUTPUT, TCSETAW, &old_term);
#else
	ioctl(TTY_OUTPUT, TIOCSETN, &old_arg);
	ioctl(TTY_OUTPUT, TIOCSETC, &old_targ);
	ioctl(TTY_OUTPUT, TIOCSLTC, &old_ltarg);
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */
    }

    tty_mode = mode;
}


int
tty_get_mode()
{
    return tty_mode;
}


/*
 * Set the interrupt character.
 */
void
tty_set_interrupt_char(c)
    int c;
{
#ifdef HAVE_POSIX_TTY
    struct termios current_term;

    tcgetattr(TTY_OUTPUT, &current_term);
    current_term.c_cc[VINTR] = c;
    current_term.c_cc[VQUIT] = CDISABLE;
    tcsetattr(TTY_OUTPUT, TCSADRAIN, &current_term);
#else
#ifdef HAVE_SYSTEMV_TTY
    struct termio current_term;

    ioctl(TTY_OUTPUT, TCGETA, &current_term);
    current_term.c_cc[VINTR] = c;
    current_term.c_cc[VQUIT] = CDISABLE;
    ioctl(TTY_OUTPUT, TCSETAW, &current_term);
#else
    struct tchars current_targ;

    ioctl(TTY_OUTPUT, TIOCGETC, &current_targ);
    current_targ.t_intrc = c;
    current_targ.t_quitc = CDISABLE;
    ioctl(TTY_OUTPUT, TIOCSETC, &current_targ);
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

    tty_interrupt_char = c;
}


/*
 * Flush the tty cache.
 */
void
tty_flush()
{
    int bytes_transferred = 0;

    while (bytes_transferred < tty_index)
    {
	int count = xwrite(TTY_OUTPUT,
			   tty_cache + bytes_transferred,
			   tty_index - bytes_transferred);
	if (count < 0)
	    break;

	bytes_transferred += count;
    }

    tty_index = 0;
}


/*
 * Write a character to the screen.  Used by tputs() to output
 * characters.  Actually we are only storing them in a buffer
 * (tty_cache[]) and flush them later (in tty_flush()).
 */
int
tty_writec(c)
    int c;
{
    if (tty_index == TTY_CACHE_SIZE)
	tty_flush();

    tty_cache[tty_index++] = (char)c;
    return 1;
}


/*
 * Send the `cl' sequence to the terminal.
 */
void
tty_io_clear()
{
    tputs(TTY_CLEAR_SCREEN, tty_lines, tty_writec);
    tty_flush();
}

/* uses the ti/te capability to signal we are entering/exiting a cursor */
/* addressable app (which saves/restores the screen, at least on xterm) */
void
tty_start_cursorapp()
{
    tputs(TTY_START_CURSORAPP,tty_lines-1,tty_writec);
    tty_flush();
}

void
tty_end_cursorapp()
{
    tty_io_clear();
    tputs(TTY_END_CURSORAPP,tty_lines-1,tty_writec);
    tty_flush();
}


/*
 * This function is called to restore the canonic mode at exit.  It also
 * clears the screen (or restore it, if possible) and sets the default
 * attributes.  If screen is NULL, there was an error, so don't try to
 * restore it.
 */
void
tty_end(screen)
    char *screen;
{
    if (tty_mode == TTY_NONCANONIC)
	tty_set_mode(TTY_CANONIC);

    tty_defaults();

#ifdef HAVE_LINUX
    if(screen && LinuxConsole)
	tty_put_screen(screen);
#endif
    tty_end_cursorapp();
    tty_io_goto(tty_lines, 0);
    tty_flush();
    printf("\n");
}


/*
 * Converts a key sequence from the human readable, '^' based form into a
 * machine usable form.
 */
char *
tty_key_human2machine(key_seq)
    unsigned char *key_seq;
{
    unsigned char *first;
    unsigned char *second;

    first = second = key_seq;

    if (tty_kbdmode == TTY_RESTRICTED_INPUT && *key_seq != '^')
	return NULL;

    while (*second)
    {
	if (*second == '^')
	{
	    if (*++second)
	    {
		/* ^G is the interrupt character and ^Z the suspend one.
		   They are not allowed in key sequences.  */
		if (toupper(*second) == 'G' || toupper(*second) == 'Z')
		    return NULL;

		*first++ = key_ctrl_table[(*second++ & 0x7F) - ' '];
	    }
	    else
		return NULL;
	}
	else
	    *first++ = *second++;
    }

    *first = 0;
    return (char *)key_seq;
}


/*
 * Converts a partial key sequence from the machine form into the human
 * readable, '^' based form.
 */
unsigned char *
tty_key_machine2human(key_seq)
    char *key_seq;
{
    unsigned char *ptr;

    keystr[0] = '\0';

    for (ptr = (unsigned char *)key_seq; *ptr; ptr++)
    {
	if (ptr != (unsigned char *)key_seq)
	    strcat((char *)keystr, " ");

	if (*ptr == key_ESC)
	    strcat((char *)keystr, "escape");
	else if (*ptr == ' ')
	    strcat((char *)keystr, "space");
	else if (*ptr == key_BACKSPACE)
	    strcat((char *)keystr, "backspace");
	else if (*ptr == key_CTRL_SPACE)
	    strcat((char *)keystr, "^space");
	else if (iscntrl(*ptr))
	{
	    char x[3];
	    x[0] = '^';
	    x[1] = *ptr + 'A' - 1;
	    x[2] = '\0';
	    strcat((char *)keystr, x);
	}
	else
	{
	    char x[2];
	    x[0] = *ptr;
	    x[1] = '\0';
	    strcat((char *)keystr, x);
	}
    }

    return (unsigned char *)keystr;
}


/*
 * Update the tty screen.
 */
void
tty_update()
{
    int pos, x, y;
    int tty_io_cursor_x = -1;
    int tty_io_cursor_y = -1;
    int last_pos = tty_columns * tty_lines;

    /* Check if we should display the last character on the screen.  */
    if (tty_last_char_flag == OFF)
	last_pos--;

    /* Make the cursor invisible.  */
    tty_cursor(OFF);

    for (pos = 0; pos < last_pos; pos++)
	if (tty_scr[pos] != tty_prev_scr[pos] ||
	    tty_atr[pos] != tty_prev_atr[pos])
	{
	    /* Move the cursor to the appropriate position, if
	       necessary.  */
	    y = pos / tty_columns;
	    x = pos % tty_columns;

	    if (x != tty_io_cursor_x || y != tty_io_cursor_y)
		tty_io_goto(tty_io_cursor_y = y, tty_io_cursor_x = x);

	    /* Output the color sequence, if necessary, then the
	       character.  */
	    tty_io_colors(tty_atr[pos]);
	    tty_writec(tty_scr[pos]);

	    if (++tty_io_cursor_x == tty_columns)
	    {
		/* Force a call to tty_io_goto() at the next iteration.
		   We don't trust the tty to do it the right way.  */
		tty_io_cursor_x = 0;
		tty_io_cursor_y = -1;
	    }
	}

    /* Update the latest cursor position.  */
    tty_io_goto(tty_cursor_y, tty_cursor_x);

    /* Make the cursor visible again.  */
    tty_cursor(ON);

    if (tty_index)
	tty_flush();

    /* Synchronize the screen copies.  */
    memcpy(tty_prev_scr, tty_scr, tty_columns * tty_lines * sizeof(char));
    memcpy(tty_prev_atr, tty_atr, tty_columns * tty_lines * sizeof(char));
}


/*
 * Add a string to the tty cache.  We implicitly assume that there will
 * be no attempt to write more than TTY_CACHE_SIZE character in a single
 * call.
 */
static int
tty_writes(s, len)
    char *s;
    int len;
{
    if (tty_index + len >= TTY_CACHE_SIZE)
	tty_flush();

    memcpy(tty_cache + tty_index, s, len);
    tty_index += len;
    return len;
}


/*
 * Write a string to the screen, at the current cursor position.
 * If the string is too long to fit between the current cursor
 * position and the end of the line, it is truncated (i.e. it doesn't
 * wrap arround).  Return the number of characters written.
 */
int
tty_puts(buf, length)
    char *buf;
    int length;
{
    int tty_offset;
    int x = tty_cursor_x;

    tty_cursor_x += length;

    if (x >= tty_columns)
	return 0;

    if (tty_cursor_y >= tty_lines)
	return 0;

    /* If the string doesn't fit completely, adjust the length.  */
    if (x + length > tty_columns)
	length = tty_columns - x;

    tty_offset = (tty_cursor_y * tty_columns) + x;

    memcpy(tty_scr + tty_offset, buf, length);
    memset(tty_atr + tty_offset, tty_current_attribute, length);

    return length;
}


/*
 * Write a character to the screen.
 */
int
tty_putc(c)
    int c;
{
    char character = c;
    return tty_puts(&character, sizeof(char));
}


/*
 * Read data from the terminal.
 */
int
tty_read(buf, length)
    char *buf;
    int length;
{
    int bytes;

    tty_update();

    if (tty_enter_idle_hook)
	(*tty_enter_idle_hook)();

    bytes = xread(TTY_INPUT, buf, length);

    if (tty_exit_idle_hook)
	(*tty_exit_idle_hook)();

    return bytes;
}


/*
 * Clear the screen using the current color attributes.  When the
 * Linux console receives the 'cl' (clear screen) escape sequence, it
 * clears the screen using the current color attributes, while all the
 * xterms I've seen seem not to do so.  Therefore, we won't rely on
 * this feature under Linux, to avoid potential problems, because I
 * don't know which approach is correct.
 */
void
tty_clear()
{
    tty_io_clear();

    memset(tty_scr,      '\0', tty_lines*tty_columns * sizeof(unsigned char));
    memset(tty_atr,      '\0', tty_lines*tty_columns * sizeof(unsigned char));
    memset(tty_prev_scr, '\0', tty_lines*tty_columns * sizeof(unsigned char));
    memset(tty_prev_atr, '\0', tty_lines*tty_columns * sizeof(unsigned char));

    tty_cursor_x = 0;
    tty_cursor_y = 0;
}


/*
 * Fill the terminal screen with spaces & the current attribute.
 */
void
tty_fill()
{
    memset(tty_scr, ' ',
	   tty_lines * tty_columns * sizeof(unsigned char));
    memset(tty_atr, tty_current_attribute,
	   tty_lines * tty_columns * sizeof(unsigned char));

    tty_touch();
}


/*
 * Touch the tty, getting rid of any possible optimization.  The
 * current content of the screen will be completely different at
 * update time so that the entire screen will be updated.
 */
void
tty_touch()
{
    memset(tty_prev_scr, '\0', tty_lines*tty_columns * sizeof(unsigned char));
}


/*
 * Move the cursor.
 */
static void
tty_io_goto(y, x)
    int y, x;
{
    /* If the 'ms' flag is present, reset all the attributes before moving
       the cursor.  */
    if (TTY_MS_FLAG == 0)
	tty_defaults();

    /* Sanity checking.  */
    if (x < 0 || x >= tty_columns ||
	y < 0 || y >= tty_lines)
	tputs(tgoto(TTY_CURSOR_MOVE, tty_columns - 1, tty_lines - 1),
	      1, tty_writec);
    else
	tputs(tgoto(TTY_CURSOR_MOVE, x, y), 1, tty_writec);
}


/*
 * Set the foreground color. Use the ANSI color sequence where possible or
 * tty_reverse() for monochrome terminals.
 */
static void
tty_io_foreground(color)
    int color;
{
    char str[16];

    if (fg_cache == VALID_CACHE && color == TTY_IO_FOREGROUND)
	return;

    if (AnsiColors == ON)
    {
	memcpy(str, ansi_foreground, sizeof(ansi_foreground));
	str[3] += color;
	tty_writes(str, sizeof(ansi_foreground));
    }
    else
	tty_io_reversevid(color != WHITE);

    fg_cache = VALID_CACHE;

    TTY_IO_SET_FOREGROUND(color);
}


/*
 * Set the background color. Use the ANSI color sequence where possible or
 * tty_reverse() for monochrome terminals.
 */
static void
tty_io_background(color)
    int color;
{
    char str[16];

    if (bg_cache == VALID_CACHE && color == TTY_IO_BACKGROUND)
	return;

    if (AnsiColors == ON)
    {
	memcpy(str, ansi_background, sizeof(ansi_background));
	str[3] += color;
	tty_writes(str, sizeof(ansi_background));
    }
    else
	tty_io_reversevid(color != BLACK);

    bg_cache = VALID_CACHE;

    TTY_IO_SET_BACKGROUND(color);
}


/*
 * Set the brightness status. See the comment below.
 */
static void
tty_io_brightness(status)
    int status;
{
    if (br_cache == VALID_CACHE && status == TTY_IO_BRIGHTNESS)
	return;

    if (status == ON)
    {
	if (TTY_BRIGHT_ON)
	    tputs(TTY_BRIGHT_ON, 1, tty_writec);
    }
    else
    {
	if (TTY_ATTRIBUTES_OFF)
	    tputs(TTY_ATTRIBUTES_OFF, 1, tty_writec);

	fg_cache = INVALID_CACHE;
	bg_cache = INVALID_CACHE;

	TTY_IO_SET_BRIGHTNESS(OFF);

	/*
	 * There is no terminal capability to turn the brightness off (or
	 * the bold mode off).  We are using the 'me' capability (where
	 * available) which turns off all attributes so we must restore
	 * the reverse status after that.
	 *
	 * There is no need to restore the foreground & background colors
	 * because we've always put tty_io_brightness(status) _before_
	 * tty_io_foreground(color) or tty_io_background(color).
	 */
	if (TTY_IO_REVERSEVID == ON)
	{
	    rv_cache = INVALID_CACHE;
	    tty_io_reversevid(ON);
	}
    }

    br_cache = VALID_CACHE;

    TTY_IO_SET_BRIGHTNESS(status);
}


/*
 * Set the reverse video status. This is only used internally by the
 * code in this file therefore it is declared 'static'.
 */
static void
tty_io_reversevid(status)
    int status;
{
    if (rv_cache == VALID_CACHE && status == TTY_IO_REVERSEVID)
	return;

    if (status == ON)
    {
	if (TTY_REVERSE_ON)
	    tputs(TTY_REVERSE_ON, 1, tty_writec);
    }
    else
    {
	if (TTY_ATTRIBUTES_OFF)
	    tputs(TTY_ATTRIBUTES_OFF, 1, tty_writec);

	fg_cache = INVALID_CACHE;
	bg_cache = INVALID_CACHE;

	TTY_IO_SET_REVERSEVID(OFF);

	/* Same comment as in tty_brightness().  */
	if (TTY_IO_BRIGHTNESS == ON)
	{
	    br_cache = INVALID_CACHE;
	    tty_io_brightness(ON);
	}
    }

    rv_cache = VALID_CACHE;

    TTY_IO_SET_REVERSEVID(status);
}


/*
 * Set the brightness, foreground and background altogether.
 */
static void
tty_io_colors(attributes)
    int attributes;
{
    tty_io_brightness(_TTY_BRIGHTNESS(attributes));
    tty_io_foreground(_TTY_FOREGROUND(attributes));
    tty_io_background(_TTY_BACKGROUND(attributes));
}


/*
 * Move the cursor.
 */
void
tty_goto(y, x)
    int y, x;
{
    tty_cursor_y = y;
    tty_cursor_x = x;
}


/*
 * Return the current coordinates of the cursor (line/column).
 */
void
tty_get_cursor(y, x)
    int *y, *x;
{
    *y = tty_cursor_y;
    *x = tty_cursor_x;
}


/*
 * Set the foreground color.
 */
void
tty_foreground(color)
    int color;
{
    TTY_SET_FOREGROUND(color);
}


/*
 * Set the background color.
 */
void
tty_background(color)
    int color;
{
    TTY_SET_BACKGROUND(color);
}


/*
 * Set the brightness status. See the comment below.
 */
void
tty_brightness(status)
    int status;
{
    TTY_SET_BRIGHTNESS(status);
}


/*
 * Set the reverse video status. This is only used internally by the
 * code in this file therefore it is declared 'static'.
 */
void
tty_reversevid(status)
    int status;
{
    TTY_SET_REVERSEVID(status);
}


/*
 * Set the brightness, foreground and background all together.
 */
void
tty_colors(brightness, foreground, background)
    int brightness, foreground, background;
{
    tty_brightness(brightness);
    tty_foreground(foreground);
    tty_background(background);
}


/*
 * Beep :-)
 */
void
tty_beep()
{
    tty_writec(7);
    tty_flush();
}


/*
 * Set the cursor status (where possible). Make it invisible during screen
 * refreshes and restore it afterward.
 */
void
tty_cursor(status)
    int status;
{
    if (status)
    {
	if (TTY_CURSOR_ON)
	    tputs(TTY_CURSOR_ON, 1, tty_writec);
    }
    else
    {
	if (TTY_CURSOR_OFF)
	    tputs(TTY_CURSOR_OFF, 1, tty_writec);
    }
}


/*
 * Store the software terminal status in a tty_status_t structure.
 */
void
tty_save(status)
    tty_status_t *status;
{
    *status = tty_current_attribute;
}


/*
 * Restore the software terminal status from a tty_status_t structure.
 */
void
tty_restore(status)
    tty_status_t *status;
{
    tty_current_attribute = *status;
}


/*
 * Restore the terminal to its default state.
 */
void
tty_defaults()
{
    if (AnsiColors == ON)
	tty_writes(ansi_defaults, sizeof(ansi_defaults));

    if (TTY_ATTRIBUTES_OFF)
	tputs(TTY_ATTRIBUTES_OFF, 1, tty_writec);

    fg_cache = INVALID_CACHE;
    bg_cache = INVALID_CACHE;
    br_cache = INVALID_CACHE;
    rv_cache = INVALID_CACHE;
}


/*
 * Extract the first key in the buffer.  If the 8th bit is set, return
 * an ESC char first, then the key without the 8th bit.
 *
 * FIXME: It seems that the 8th bit can be used for parity as well.
 * This case is not handled yet.  It should be configurable.
 */
static int
tty_extract_key()
{
    int key = keybuf[keyindex];

    if (key & 0x80)
    {
	keybuf[keyindex] &= 0x7F;
	return key_ESC;
    }

    keyno--;
    keyindex++;
    return key;
}


/*
 * Get a character from the terminal.  For better performance on
 * highly loaded systems, read all the data available.
 */
int
tty_getc()
{
    service_pending_signals();

    if (keyno)
	return tty_extract_key();

    /* No interrupt/quit character.  */
    tty_set_interrupt_char(-1);

    /* Allow signals to suspend/resize git.  */
    signals(ON);

    keyindex = 0;
    while ((keyno = tty_read(keybuf, 1024)) < 0)
	;

    /* Prevent signals from suspending/resizing git.  */
    signals(OFF);

    /* Restore ^G as the interrupt/quit character.  */
    tty_set_interrupt_char(key_INTERRUPT);

    return keyno ? tty_extract_key() : -1;
}


/*
 * Insert a key sequence into the list.
 */
void
tty_key_list_insert_sequence(key, key_seq, aux_data)
    tty_key_t **key;
    unsigned char *key_seq;
    void *aux_data;
{
    tty_key_t *new_key;

    new_key = (tty_key_t *)xmalloc(sizeof(tty_key_t));
    new_key->key_seq = (unsigned char *)xstrdup((char *)key_seq);
    new_key->aux_data = aux_data;
    new_key->next = *key;
    *key = new_key;
}


/*
 * Parse the key list, inserting the new key sequence in the proper
 * position.
 */
void
tty_key_list_insert(key_seq, aux_data)
    unsigned char *key_seq;
    void *aux_data;
{
    static tty_key_t **key = NULL;

    if (*key_seq == 0)
	return;               /* bad key sequence !  */

    /* Try to optimize by checking if the current entry should be added
       after the current position.  Avoid re-parsing the entire list if
       so.  */
    if (key == NULL || strcmp((char *)key_seq, (char *)(*key)->key_seq) <= 0)
	key = &key_list_head;

    for (; *key; key = &(*key)->next)
	if (strcmp((char *)key_seq, (char *)(*key)->key_seq) <= 0)
	{
	    tty_key_list_insert_sequence(key, key_seq, aux_data);
	    return;
	}

    tty_key_list_insert_sequence(key, key_seq, aux_data);
}


/*
 * Force the next key search to start from the begining of the list.
 */
void
tty_key_search_restart()
{
    current_key = key_list_head;
}


/*
 * Incremental searches a key in the list. Returns a pointer to the first
 * sequence that matches, -1 if there is no match and NULL if there can
 * still be a match.
 */
tty_key_t *
tty_key_search(key_seq)
    char *key_seq;
{
    int cmp;

    if (current_key == NULL)
	return NULL;

    for (; current_key; current_key = current_key->next)
    {
	cmp = strcmp(key_seq, (char *)current_key->key_seq);

	if (cmp == 0)
	    return current_key;

	if (cmp  < 0)
	    break;
    }

    if (current_key == NULL ||
	strncmp(key_seq, (char *)current_key->key_seq, strlen(key_seq)) != 0)
	return (tty_key_t *)-1;
    else
	return NULL;
}


#if 0
/*
 * Delete a key from the list. Don't remove this function, God only knows
 * when I'm gonna need it...
 */
void
tty_key_list_delete()
{
    tty_key_t *key, *next_key;

    for (key = key_list_head; key; key = next_key)
    {
	next_key = key->next;
	xfree(key->key_seq);
	xfree(key);
    }
}
#endif


/*
 * Print the key sequence on the last line of the screen.
 */
void
tty_key_print(key_seq)
    char *key_seq;
{
    tty_status_t tty_status;
    char *typed = "Keys typed so far: ";
    char *incomplete = " ";
    char *spaces;

    tty_save(&tty_status);
    tty_goto(tty_lines - 1, 0);
    tty_background(WHITE);
    tty_foreground(BLACK);

    spaces = xmalloc(tty_columns+1);
    memset(spaces, ' ', tty_columns);
    spaces[tty_columns] = '\0';
    tty_puts(spaces, tty_columns);
    xfree(spaces);
    tty_goto(tty_lines - 1, 0);

    tty_key_machine2human(key_seq);

    tty_puts(typed, strlen(typed));
    tty_puts((char *)keystr, strlen((char *)keystr));
    tty_puts(incomplete, strlen(incomplete));

    tty_update();
    tty_restore(&tty_status);
}


/*
 * Get the first key available, returning also the repeat count, that
 * is, the number of times the key has been pressed.  These feature is
 * mainly used by the calling routines to perform optimizations.  For
 * example, if you press the down arrow several times, the caller can
 * display only the final position, saving a lot of time. If you have
 * ever worked with git on highly loaded systems, I'm sure you know what
 * I mean.
 *
 * If tty_get_key() returns NULL, the key sequence is invalid.
 */
tty_key_t *
tty_get_key(repeat_count)
    int *repeat_count;
{
    int i, c;
    tty_key_t *key = NULL;

    while ((c = tty_getc()) == -1)
	;

    if (repeat_count)
	*repeat_count = 1;

    /* Handle ^SPC.  */
    if (c == 0)
	c = 0xff;

    if (tty_kbdmode == TTY_RESTRICTED_INPUT)
    {
	if (c == '\n' || c == '\r')
	    c = key_ENTER;

	if (isprint(c) || c == key_INTERRUPT)
	{
	    default_key.key_seq[0] = c;
	    default_key.key_seq[1] = '\0';
	    return &default_key;
	}
    }

    partial = 0;
    key_on_display = 0;

    tty_key_search_restart();

    for (i = 0; i < MAX_KEY_LENGTH; i++)
    {
	/* Kludge to handle brain-damaged key sequences.  If a 0
	   (^SPC) is found, change it into 0xff.  I don't know how
	   smart this is, but right know I don't feel like doing it
	   otherwise.  */
	if (c == 0)
	    c = 0xff;

	tty_key_seq[i] = c;
	tty_key_seq[i + 1] = 0;

	key = tty_key_search((char *)tty_key_seq);

	if (key == (tty_key_t *)-1)
	{
	    alarm(1);
	    partial = 0;
	    return NULL;
	}

	if (key)
	{
	    if (partial)
	    {
		/* tty_key_print((char *)tty_key_seq); */
		/* tty_update(); */
		/* Small delay for displaying the selected sequence.  */
		/* FIXME: 1 second is way too much!  */
		/* sleep(1);  */
	    }
	    break;
	}

	if (keyno == 0)
	{
	    /* Schedule an alarm in 1 second, to display the key
	       sequence if still incomplete by that time.  */
	    if (key_on_display)
		tty_key_print((char *)tty_key_seq);
	    else
		alarm(1);
	    partial = 1;
	}

	while ((c = tty_getc()) == -1)
	    ;
    }

    if (i == MAX_KEY_LENGTH)
    {
	alarm(1);
	partial = 0;
	return NULL;
    }

    if (repeat_count)
	while (keyno > i &&
	       (memcmp(tty_key_seq, &keybuf[keyindex], i + 1) == 0))
	{
	    keyindex += i + 1;
	    keyno -= i + 1;
	    (*repeat_count)++;
	}

    alarm(1);
    partial = 0;
    return key;
}


void
tty_key_print_async()
{
    if (partial)
    {
	tty_key_print((char *)tty_key_seq);
	key_on_display = 1;
    }
}


char *
tty_get_previous_key_seq()
{
    return (char *)tty_key_seq;
}


#define columns_ok(x) (((x) > 0) && ((x) <= MAX_TTY_COLUMNS))
#define lines_ok(x)   (((x) > 0) && ((x) <= MAX_TTY_LINES))


/*
 * Retrieve the screen size.  The default is to use the number of
 * lines and columns specified in the environment.  If the values are
 * not acceptable, default to the value returned by the winsz ioctl()
 * call.  If these ones are not valid (or the ioctl() call to get the
 * window size is not supported), just default to the values specified
 * in the termcap description.  If this fails too, default to 80x24.
 * Note that we try to get the best value separately for lines and for
 * columns.
 */
void
tty_resize()
{
    char *env;
    char buf[32];
    int shell_lines = 0, shell_columns = 0;
    int termcap_lines = 0, termcap_columns = 0;

#ifdef HAVE_WINSZ
    struct winsize winsz;
    int winsz_lines = 0, winsz_columns = 0;
#endif /* HAVE_WINSZ */

#ifdef HAVE_WINSZ
#if defined TIOCGSIZE && !defined TIOCGWINSZ
#define TIOCGWINSZ TIOCGSIZE
#endif
    if (ioctl(TTY_OUTPUT, TIOCGWINSZ, &winsz) == 0)
	if (winsz.ws_col && winsz.ws_row)
	{
	    winsz_columns = winsz.ws_col;
	    winsz_lines   = winsz.ws_row;
	}
#endif /* HAVE_WINSZ */

    if ((env = getenv("COLUMNS")))
	sscanf(env, "%d", &shell_columns);

    if ((env = getenv("LINES")))
	sscanf(env, "%d", &shell_lines);

    termcap_columns = TTY_COLUMNS;
    termcap_lines   = TTY_LINES;

#ifdef HAVE_WINSZ
    if (columns_ok(winsz_columns))
	tty_columns = winsz_columns;
    else
    {
#endif
	if (columns_ok(shell_columns))
	    tty_columns = shell_columns;
	else if (columns_ok(termcap_columns))
	    tty_columns = termcap_columns;
	else
	    tty_columns = 80;
#ifdef HAVE_WINSZ
    }
#endif

#ifdef HAVE_WINSZ
    if (lines_ok(winsz_lines))
	tty_lines = winsz_lines;
    else
    {
#endif
	if (lines_ok(shell_lines))
	    tty_lines = shell_lines;
	else if (lines_ok(termcap_lines))
	    tty_lines = termcap_lines;
	else
	    tty_lines = 24;
#ifdef HAVE_WINSZ
    }
#endif

    assert(tty_lines != 0);
    assert(tty_columns != 0);

    /* Update the LINES & COLUMNS environment variables to reflect the
       change in the window size.  This is important in order to avoid
       passing children incorrect environment values.  We only do this
       when we trust the values we've got (i.e. when we got them
       through TIOCGWINSZ).  Thanks to xax@roedu.net for the
       suggestion.  */
    sprintf(buf, "%d", tty_lines);
    xsetenv("LINES", buf);
    sprintf(buf, "%d", tty_columns);
    xsetenv("COLUMNS", buf);

    /* Resize the tty buffers.  */
    if (tty_scr)
	xfree(tty_scr);

    if (tty_atr)
	xfree(tty_atr);

    if (tty_prev_scr)
	xfree(tty_prev_scr);

    if (tty_prev_atr)
	xfree(tty_prev_atr);

    tty_scr = (unsigned char *)xcalloc(
	tty_columns * tty_lines, sizeof(unsigned char));
    tty_atr = (unsigned char *)xcalloc(
	tty_columns * tty_lines, sizeof(unsigned char));

    tty_prev_scr = (unsigned char *)xcalloc(
	tty_columns * tty_lines, sizeof(unsigned char));
    tty_prev_atr = (unsigned char *)xcalloc(
	tty_columns * tty_lines, sizeof(unsigned char));

#ifdef SIGWINCH
    /* We need to pass the resize command to the parent process
       (usually the shell).  */
    /* FIXME: This doesn't work and I haven't got the time to look
       into it...  */
    /*kill(getppid(), SIGWINCH);*/
#endif
}


#ifdef HAVE_LINUX
/*
 * Read and write the screen contents via a Linux virtual console.
 * Returns 0 on failure, non-zero otherwise.  Handle both /dev/vcsaX
 * and /dev/vcsX.  op specifies the operation to be performed: VCS_READ
 * or VCS_WRITE.  We check first for /dev/vcs?X since it makes debugging
 * easier (using /de/vcsa0 when debugging dumps the screen on the
 * debugger's console; not funny).
 */
int
vcs_io(buf, op)
    char *buf;
    int op;
{
    ssize_t (*fn)();
    int vcsfd, flag;
    char vcs_name[16];
    char vcsa_name[16];

    strcpy(vcs_name, "/dev/vcsXX");
    strcpy(vcsa_name, "/dev/vcsaXX");

    if (op == VCS_READ)
    {
	flag = O_RDONLY;
	fn = read;
    }
    else
    {
	flag = O_WRONLY;
	fn = write;

	if (vcs_is_monochrome)
	    goto monochrome;
    }

    vcs_is_monochrome = 0;

    /* First attempt: /dev/vcsaX.  */
    vcsa_name[9] = tty_device[8];
    vcsa_name[10] = tty_device[9];
    vcsfd = open(vcsa_name, flag);

    if (vcsfd != -1)
    {
      vcsa_label:
	(*fn)(vcsfd, buf, 4 + tty_lines * tty_columns * 2);
	close(vcsfd);

	if (op == VCS_WRITE)
	{
	    tty_io_goto(buf[3], buf[2]);
	    tty_flush();
	}

	return 1;
    }

    /* Second attempt: /dev/vcsa0.  */
    vcsa_name[9] = '0';
    vcsa_name[10] = '\0';
    vcsfd = open(vcsa_name, flag);

    if (vcsfd != -1)
	goto vcsa_label;

  monochrome:
    /* We failed to access a /dev/vcsa* device.  */
    vcs_is_monochrome = 1;

    /* This is important in order to clear the screen attributes.  */
    if (op == VCS_WRITE)
	tty_clear();

    /* Third attempt: /dev/vcsX (B/W).  */
    vcs_name[8] = tty_device[8];
    vcs_name[9] = tty_device[9];
    vcsfd = open(vcs_name, flag);

    if (vcsfd != -1)
    {
      vcs_label:
	(*fn)(vcsfd, buf, 4 + tty_lines * tty_columns);
	close(vcsfd);

	/* Unfortunately, the b/w devices do not provide the 4 bytes header
	   with screen size & cursor position information.  */

	if (op == VCS_WRITE)
	{
	    tty_io_goto(tty_lines - 1, 0);
	    tty_flush();
	}
	return 1;
    }

    /* Fourth attempt: /dev/vcs0 (B/W).  */
    vcs_name[8] = '0';
    vcs_name[9] = '\0';
    vcsfd = open(vcs_name, flag);

    if (vcsfd != -1)
	goto vcs_label;

    return 0;
}
#endif


/*
 * Dump the screen. Only used in Linux if the terminal is a virtual
 * console.
 */
void
tty_get_screen(buf)
    char *buf;
{
#ifdef HAVE_LINUX
    if (LinuxConsole)
	vcs_read_ok = vcs_io(buf, VCS_READ);
#else
    buf = NULL;
#endif  /* HAVE_LINUX */
}


/*
 * Restore the screen. If the terminal is not a Linux virtual console, just
 * restore it to the default state.
 */

void
tty_put_screen(buf)
    char *buf;
{
    tty_defaults();

#ifdef HAVE_LINUX
    if (LinuxConsole)
    {
	/* If we were unable to read from /dev/vcs*, then we should not
	   try to restore it.  */
	if (vcs_read_ok)
	{
	    tty_touch();

	    /* We might have the permission to read the contents of the
	       virtual console, but not the permission to write it.  */
	    if (vcs_io(buf, VCS_WRITE) == 0)
		tty_clear();
	    else
		memset(tty_scr, '\0',
		       tty_lines * tty_columns * sizeof(unsigned char));
	}
	else
	    tty_clear();
    }
    else
	tty_clear();
#else   /* !HAVE_LINUX */
    tty_clear();
    buf = NULL;
#endif  /* !HAVE_LINUX */
}


/*
 * Get the color index from the colors[] index table.
 */
int
tty_get_color_index(colorname)
    char *colorname;
{
    int i;

    for (i = 0; i < 10; i++)
	if (strcmp(colors[i], colorname) == 0)
	    return (i < 8) ? i : (i - 8);

    return -1;
}


/*
 * Get the capability of a given termcap symbol.  Return NULL if there
 * is no capability for it.
 */
char *
tty_get_symbol_key_seq(symbol)
    char *symbol;
{
    int i;

    for (i = TTY_FIRST_SYMBOL_KEY; i < TTY_CAPABILITIES_USED; i++)
	if (strcmp(tty_capability[i].symbol, symbol) == 0)
	    return tty_capability[i].string;

    return NULL;
}


/*
 * Get the entire set of required termcap/terminfo capabilities. It performs
 * consistency checkings trying to recover as well as possible.
 */
void
tty_get_capabilities()
{
#ifdef HAVE_LINUX
    struct stat statbuf;
#endif /* HAVE_LINUX */
    char *capability_buf, *tmp;
    int err, i, term_errors = 0;
    char *termtype = getenv("TERM");

#ifdef HAVE_LINUX
    fstat(TTY_OUTPUT, &statbuf);
    /* dropped test for minor <= 8 - linux now has unlimited VCs */
    if ((statbuf.st_rdev >> 8) == LINUX_VC_MAJOR)
	LinuxConsole = 1;
    else
	LinuxConsole = 0;
#endif /* HAVE_LINUX */

    if (termtype == NULL)
    {
	fprintf(stderr, "%s: can't find the TERM environment variable, ",
		g_program);
	goto switch_to_vt100;
    }

    if (strlen(termtype) > 63)
    {
	fprintf(stderr, "%s: the TERM environment variable is too long, ",
		g_program);
      switch_to_vt100:
	fprintf(stderr, "trying vt100 ...\n");
	termtype = vt100;
    }

  retry:
#ifdef HAVE_LINUX
    err = tgetent(NULL, termtype);
#else
    err = tgetent(term_buf, termtype);
#endif

    if (err == -1)
    {
	fprintf(stderr, "%s: can't find the %s database.\n",
		g_program, term_database);
	fprintf(stderr, "%s: check your %s environment variable ...\n",
		g_program, term_env);
	exit(1);
    }

    if (err == 0)
    {
	fprintf(stderr,
		"%s: can't find the terminal type %s in the %s database.\n",
		g_program, termtype, term_database);

	if (strcmp(termtype, "iris-ansi") == 0)
	{
	    fprintf(stderr, "%s: trying ansi...\n", g_program);
	    termtype = "ansi";
	    goto retry;
	}
	
	if (tty_is_xterm(termtype))
	{
	    fprintf(stderr, "%s: trying xterm...\n", g_program);
	    termtype = "xterm";
	    goto retry;
	}

	if (strcmp(termtype, "vt220") == 0 ||
	    strcmp(termtype, "vt320") == 0)
	{
	    fprintf(stderr, "%s: trying vt100...\n", g_program);
	    termtype = "ansi";
	    goto retry;
	}

	exit(1);
    }

    tty_type = xstrdup(termtype);

    capability_buf = xmalloc(2048);

    tmp = tgetstr(TTY_PAD_CHAR_NAME, &capability_buf);
    PC = tmp ? *tmp : 0;

    BC = tgetstr(TTY_LEFT_ONE_SPACE_NAME, &capability_buf);
    UP = tgetstr(TTY_UP_ONE_LINE_NAME,    &capability_buf);

    if (BC == NULL || UP == NULL)
	BC = UP = NULL;

    TTY_ATTRIBUTES_OFF = tgetstr(TTY_ATTRIBUTES_OFF_NAME, &capability_buf);
    TTY_BRIGHT_ON      = tgetstr(TTY_BRIGHT_ON_NAME,      &capability_buf);
    TTY_REVERSE_ON     = tgetstr(TTY_REVERSE_ON_NAME,     &capability_buf);

    if (TTY_ATTRIBUTES_OFF == NULL)
	TTY_REVERSE_ON = TTY_BRIGHT_ON = NULL;

    TTY_STANDOUT_ON  = tgetstr(TTY_STANDOUT_ON_NAME,  &capability_buf);

    if (TTY_STANDOUT_ON == NULL)
    {
	TTY_STANDOUT_ON = NULL;
	TTY_MS_FLAG = 0;
    }
    else
    {
	/* Use standout instead of reverse video whenever possible.  */
	TTY_REVERSE_ON = TTY_STANDOUT_ON;
	TTY_MS_FLAG = tgetflag(TTY_MS_FLAG_NAME);
    }

    /* Check for magic-cookie terminals.  Not supported yet.  */
    TTY_MAGIC_COOKIE = tgetnum(TTY_MAGIC_COOKIE_NAME);

    if (TTY_MAGIC_COOKIE >= 0)
	TTY_ATTRIBUTES_OFF = TTY_REVERSE_ON = TTY_BRIGHT_ON = NULL;


    /* Try to figure out the number of lines and columns as specified
       in the termcap description.  */
    TTY_COLUMNS = tgetnum(TTY_COLUMNS_NAME);
    TTY_LINES   = tgetnum(TTY_LINES_NAME);


    TTY_CURSOR_OFF = tgetstr(TTY_CURSOR_OFF_NAME, &capability_buf);
    TTY_CURSOR_ON  = tgetstr(TTY_CURSOR_ON_NAME,  &capability_buf);

    if (TTY_CURSOR_OFF == NULL || TTY_CURSOR_ON == NULL)
	TTY_CURSOR_ON = TTY_CURSOR_OFF = NULL;

    TTY_CLEAR_SCREEN = tgetstr(TTY_CLEAR_SCREEN_NAME, &capability_buf);
    TTY_CURSOR_MOVE  = tgetstr(TTY_CURSOR_MOVE_NAME,  &capability_buf);

    TTY_START_CURSORAPP = tgetstr(TTY_START_CURSORAPP_NAME, &capability_buf);
    TTY_END_CURSORAPP = tgetstr(TTY_END_CURSORAPP_NAME, &capability_buf);

    for (i = TTY_FIRST_SYMBOL_KEY; i < TTY_CAPABILITIES_USED; i++)
	tty_capability[i].string = tgetstr(tty_capability[i].name,
					   &capability_buf);

    for (i = 0; i < TTY_CAPABILITIES_USED; i++)
	if (tty_capability[i].string == NULL)
	{
	    if (tty_capability[i].required)
	    {
		term_errors++;
		fprintf(stderr,
			"%s: can't find the '%s' terminal capability.\n",
			g_program, tty_capability[i].name);
	    }
	    else
	    {
		/* If this capability does not exist but its presence
		   is not mandatory then make it be the null string.  */
		tty_capability[i].string = "";
	    }
	}

    if (term_errors)
    {
	fprintf(stderr, "%s: %d errors. Your terminal is too dumb :-< .\n",
		g_program, term_errors);
	exit(1);
    }
}


/*
 * Initialize the tty driver.
 */
void
tty_init(kbd_mode)
    int kbd_mode;
{
    /* Fail if stdin or stdout are not real terminals.  */
    if (!isatty(TTY_INPUT) || !isatty(TTY_OUTPUT))
    {
	fprintf(stderr, "%s: only stderr can be redirected.\n", g_program);
	exit(1);
    }

    if ((tty_device = ttyname(1)) == NULL)
    {
	fprintf(stderr, "%s: can't get terminal name.\n", g_program);
	exit(1);
    }

    /* Store the terminal settings in old_term. it will be used to restore
       them later.  */
#ifdef HAVE_POSIX_TTY
    tcgetattr(TTY_OUTPUT, &old_term);
#else
#ifdef HAVE_SYSTEMV_TTY
    ioctl(TTY_OUTPUT, TCGETA, &old_term);
#else
    ioctl(TTY_OUTPUT, TIOCGETP, &old_arg);
    ioctl(TTY_OUTPUT, TIOCGETC, &old_targ);
    ioctl(TTY_OUTPUT, TIOCGLTC, &old_ltarg);
#endif /* HAVE_SYSTEMV_TTY */
#endif /* HAVE_POSIX_TTY */

    default_key.key_seq  = tty_key_seq = (unsigned char *)xmalloc(64);
    default_key.aux_data = NULL;
    default_key.next = NULL;
    tty_kbdmode = kbd_mode;

    tty_device_length = strlen(tty_device);
    tty_get_capabilities();
}


/*
 * Update the title of xterm-like X terminal emulators.
 */
void
tty_update_title(string)
    char *string;
{
    if (tty_is_xterm(tty_type))
    {
	size_t len = strlen(string);
	char *temp = xmalloc(128 + len + 1);
	char *printable_string = xstrdup(string);

	toprintable(printable_string, len);
	sprintf(temp, "%c]2;%s - %s%c", 0x1b, PRODUCT, printable_string, 0x07);

	/* I don't know what can be considered a resonable limit here,
	   I just arbitrarily chosed to truncate the length of the
	   title to twice the number of columns.  Longer strings seem
	   to make fvwm2 issue error messages.  */
	if (128 + (int)len > 2 * tty_columns)
	{
	    temp[2 * tty_columns    ] = 0x07;
	    temp[2 * tty_columns + 1] = '\0';
	}

	xwrite(TTY_OUTPUT, temp, strlen(temp));
	xfree(printable_string);
	xfree(temp);
	fflush(stdout);
    }
}

static int
tty_is_xterm(term)
    char *term;
{
    if (strncmp(term, "xterm", 5)     == 0 ||
	strncmp(term, "rxvt", 4)      == 0 ||
	strncmp(term, "iris-ansi", 9) == 0 ||
	strcmp(term, "aixterm")       == 0 ||
	strcmp(term, "Eterm")         == 0 ||
	strcmp(term, "dtterm")        == 0)
    {
	return 1;
    }
    return 0;
}
