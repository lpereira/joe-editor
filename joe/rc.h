/*
 *	*rc file parser
 *	Copyright
 *		(C) 1992 Joseph H. Allen; 
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#ifndef _JOE_RC_H
#define _JOE_RC_H 1

extern OPTIONS pdefault;
extern OPTIONS fdefault;
void setopt(B *b, unsigned char *name);

/* KMAP *kmap_getcontext(char *name);
 * Find and return the KMAP for a given context name.  If none is found, an
 * empty kmap is created, bound to the context name, and returned.
 */
KMAP *kmap_getcontext(unsigned char *name);

/* KMAP *ngetcontext(char *name);
 * JM - Find and return the KMAP for a given context name.  If none is found,
 * NULL is returned.
 */
KMAP *ngetcontext(unsigned char *name);

unsigned char **get_keymap_list();

/* int procrc(CAP *cap, char *name);  Process an rc file
   Returns 0 for success
          -1 for file not found
           1 for syntax error (errors written to stderr)
*/
int procrc(CAP *cap, unsigned char *name);

int glopt(unsigned char *s, unsigned char *arg, OPTIONS *options, int set);

int umode(BW *bw);
int umenu(BW *bw);

/* Save state */
void save_state();

/* Load state */
void load_state();

/* Validate rc file: return -1 if it's bad (call this after rc file has been loaded) */
int validate_rc();

/* Update options */
void lazy_opts(B *b, OPTIONS *o);

int ucharset(BW *bw);
int ulanguage(BW *bw);

unsigned char *get_status(BW *bw, unsigned char *s);

#endif
