/*
 *	Help system
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *		(C) 2001 Marek 'Marx' Grac
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#ifndef _JOE_HELP_H
#define _JOE_HELP_H 1

void help_display PARAMS((SCREEN *t));		/* display text in help window */
int help_on PARAMS((SCREEN *t));		/* turn help on */
int help_init PARAMS((FILE *fd,unsigned char *bf,int line)); /* read help from rc file */

int u_help PARAMS((BASE *base));		/* toggle help on/off */
int u_help_next PARAMS((BASE *base));		/* goto next help screen */
int u_help_prev PARAMS((BASE *base));		/* goto prev help screen */
extern int bg_help;				/* Background color for help */

#endif
