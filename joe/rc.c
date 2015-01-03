/*
 *	*rc file parser
 *	Copyright
 *		(C) 1992 Joseph H. Allen; 
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Commands which just type in variable values */

int ucharset(BW *bw)
{
	unsigned char *s;
	W *w=bw->parent->main;
	s=((BW *)w->object)->o.charmap->name;
	if (!s || !*s)
		return -1;
	while (*s)
		if (utypebw(bw,*s++))
			return -1;
	return 0;
}

int ulanguage(BW *bw)
{
	unsigned char *s;
	W *w=bw->parent->main;
	s=((BW *)w->object)->o.language;
	if (!s || !*s)
		return -1;
	while (*s)
		if (utypebw(bw,*s++))
			return -1;
	return 0;
}

#define OPT_BUF_SIZE 300

static struct context {
	struct context *next;
	unsigned char *name;
	KMAP *kmap;
} *contexts = NULL;		/* List of named contexts */

/* Find a context of a given name- if not found, one with an empty kmap
 * is created.
 */

KMAP *kmap_getcontext(unsigned char *name)
{
	struct context *c;

	for (c = contexts; c; c = c->next)
		if (!strcmp(c->name, name))
			return c->kmap;
	c = (struct context *) joe_malloc(sizeof(struct context));

	c->next = contexts;
	c->name = strdup(name);
	contexts = c;
	return c->kmap = mkkmap();
}

/* JM - ngetcontext(name) - like getcontext, but return NULL if it
 * doesn't exist, instead of creating a new one.
 */

KMAP *ngetcontext(unsigned char *name)
{
	struct context *c;
	for(c=contexts;c;c=c->next)
		if(!strcmp(c->name,name))
			return c->kmap;
	return 0;
}

/* Validate joerc file */

int validate_rc()
{
	KMAP *k = ngetcontext("main");
	int x;
	/* Make sure main exists */
	if (!k)
		return -1;
	/* Make sure there is at least one key binding */
	for (x = 0; x != KEYS; ++x)
		if (k->keys[x].value.bind)
			return 0;
	return -1;
}

unsigned char **get_keymap_list()
{
	unsigned char **lst = 0;
	struct context *c;
	for (c=contexts; c; c=c->next)
		lst = vaadd(lst, vsncpy(NULL,0,sz(c->name)));

	return lst;
}

OPTIONS *options = NULL;

/* Set to use ~/.joe_state file */
int joe_state;

/* Default options for prompt windows */

OPTIONS pdefault = {
	NULL,		/* *next */
	NULL,		/* *name_regex */
	NULL,		/* *contents_regex */
	0,		/* overtype */
	0,		/* lmargin */
	76,		/* rmargin */
	0,		/* autoindent */
	0,		/* wordwrap */
	0,		/* nobackup */
	8,		/* tab */
	' ',		/* indent char */
	1,		/* indent step */
	NULL,		/* *context */
	NULL,		/* *lmsg */
	NULL,		/* *rmsg */
	0,		/* line numbers */
	0,		/* read only */
	0,		/* french spacing */
	0,		/* flowed text */
	0,		/* spaces */
	0,		/* crlf */
	0,		/* Highlight */
	NULL,		/* Syntax name */
	NULL,		/* Syntax */
	NULL,		/* Name of character set */
	NULL,		/* Character set */
	NULL,		/* Language */
	0,		/* Smart home key */
	0,		/* Goto indent first */
	0,		/* Smart backspace key */
	0,		/* Purify indentation */
	0,		/* Picture mode */
	0,		/* single_quoted */
	0,		/* no_double_quoted */
	0,		/* c_comment */
	0,		/* cpp_comment */
	0,		/* pound_comment */
	0,		/* vhdl_comment */
	0,		/* semi_comment */
	0,		/* tex_comment */
	0,		/* hex */
	NULL,		/* text_delimiters */
	NULL,		/* Characters which can indent paragraphs */
	NULL,		/* macro to execute for new files */
	NULL,		/* macro to execute for existing files */
	NULL,		/* macro to execute before saving new files */
	NULL,		/* macro to execute before saving existing files */
	NULL		/* macro to execute on first change */
};

/* Default options for file windows */

OPTIONS fdefault = {
	NULL,		/* *next */
	NULL,		/* *name_regex */
	NULL,		/* *contents_regex */
	0,		/* overtype */
	0,		/* lmargin */
	76,		/* rmargin */
	0,		/* autoindent */
	0,		/* wordwrap */
	0,		/* nobackup */
	8,		/* tab */
	' ',		/* indent char */
	1,		/* indent step */
	"main",		/* *context */
	"\\i%n %m %M",	/* *lmsg */
	" %S Ctrl-K H for help",	/* *rmsg */
	0,		/* line numbers */
	0,		/* read only */
	0,		/* french spacing */
	0,		/* flowed text */
	0,		/* spaces */
	0,		/* crlf */
	0,		/* Highlight */
	NULL,		/* Syntax name */
	NULL,		/* Syntax */
	NULL,		/* Name of character set */
	NULL,		/* Character set */
	NULL,		/* Language */
	0,		/* Smart home key */
	0,		/* Goto indent first */
	0,		/* Smart backspace key */
	0,		/* Purity indentation */
	0,		/* Picture mode */
	0,		/* single_quoted */
	0,		/* no_double_quoted */
	0,		/* c_comment */
	0,		/* cpp_comment */
	0,		/* pound_comment */
	0,		/* vhdl_comment */
	0,		/* semi_comment */
	0,		/* tex_comment */
	0,		/* hex */
	NULL,		/* text_delimiters */
	">;!#%/*-",	/* Characters which can indent paragraphs */
	NULL, NULL, NULL, NULL, NULL	/* macros (see above) */
};

/* Update options */

void lazy_opts(B *b, OPTIONS *o)
{
	o->syntax = load_syntax(o->syntax_name);
	if (!o->map_name) {
		/* Guess encoding if it's not explicitly given */
		unsigned char buf[1024];
		int len = 1024;
		if (b->eof->byte < 1024)
			len = b->eof->byte;
		brmem(b->bof, buf, len);
		o->charmap = guess_map(buf, len);
		o->map_name = strdup(o->charmap->name);
	} else {
		o->charmap = find_charmap(o->map_name);
	}
	if (!o->charmap)
		o->charmap = locale_map;
	if (!o->language)
		o->language = strdup(locale_msgs);
	/* Hex not allowed with UTF-8 */
	if (o->hex && o->charmap->type) {
		o->charmap = find_charmap("c");
	}
}

/* Set local options depending on file name and contents */

void setopt(B *b, unsigned char *parsed_name)
{
	OPTIONS *o;
	int x;
	unsigned char *pieces[26];
	for (x = 0; x!=26; ++x)
		pieces[x] = NULL;

	for (o = options; o; o = o->next)
		if (rmatch(o->name_regex, parsed_name)) {
			if(o->contents_regex) {
				P *p = pdup(b->bof, "setopt");
				if (pmatch(pieces,o->contents_regex,strlen(o->contents_regex),p,0,0)) {
					prm(p);
					b->o = *o;
					lazy_opts(b, &b->o);
					goto done;
				} else {
					prm(p);
				}
			} else {
				b->o = *o;
				lazy_opts(b, &b->o);
				goto done;
			}
		}

	b->o = fdefault;
	lazy_opts(b, &b->o);

	done:
	for (x = 0; x!=26; ++x)
		vsrm(pieces[x]);
}

/* Table of options and how to set them */

/* local means it's in an OPTION structure, global means it's in a global
 * variable */

struct glopts {
	unsigned char *name;		/* Option name */
	int type;		/*      0 for global option flag
				   1 for global option numeric
				   2 for global option string
				   4 for local option flag
				   5 for local option numeric
				   6 for local option string
				   7 for local option numeric+1, with range checking
				 */
	void *set;		/* Address of global option */
	unsigned char *addr;		/* Local options structure member address */
	unsigned char *yes;		/* Message if option was turned on, or prompt string */
	unsigned char *no;		/* Message if option was turned off */
	unsigned char *menu;		/* Menu string */
	int ofst;		/* Local options structure member offset */
	int low;		/* Low limit for numeric options */
	int high;		/* High limit for numeric options */
} glopts[] = {
	{"overwrite",4, NULL, (unsigned char *) &fdefault.overtype, _("Overtype mode"), _("Insert mode"), _("T Overtype ") },
	{"hex",4, NULL, (unsigned char *) &fdefault.hex, _("Hex edit mode"), _("Text edit mode"), _("  Hex edit mode ") },
	{"autoindent",	4, NULL, (unsigned char *) &fdefault.autoindent, _("Autoindent enabled"), _("Autoindent disabled"), _("I Autoindent ") },
	{"wordwrap",	4, NULL, (unsigned char *) &fdefault.wordwrap, _("Wordwrap enabled"), _("Wordwrap disabled"), _("W Word wrap ") },
	{"tab",	5, NULL, (unsigned char *) &fdefault.tab, _("Tab width (%d): "), 0, _("D Tab width "), 0, 1, 64 },
	{"lmargin",	7, NULL, (unsigned char *) &fdefault.lmargin, _("Left margin (%d): "), 0, _("L Left margin "), 0, 0, 63 },
	{"rmargin",	7, NULL, (unsigned char *) &fdefault.rmargin, _("Right margin (%d): "), 0, _("R Right margin "), 0, 7, 255 },
	{"restore",	0, &restore_file_pos, NULL, _("Restore cursor position when files loaded"), _("Don't restore cursor when files loaded"), _("  Restore cursor ") },
	{"square",	0, &square, NULL, _("Rectangle mode"), _("Text-stream mode"), _("X Rectangle mode ") },
	{"icase",	0, &icase, NULL, _("Search ignores case by default"), _("Case sensitive search by default"), _("  Case insensitivity ") },
	{"wrap",	0, &wrap, NULL, _("Search wraps"), _("Search doesn't wrap"), _("  Search wraps ") },
	{"menu_explorer",	0, &menu_explorer, NULL, _("Menu explorer mode"), _("Simple completion mode"), _("  Menu explorer ") },
	{"menu_above",	0, &menu_above, NULL, _("Menu above prompt"), _("Menu below prompt"), _("  Menu position ") },
	{"search_prompting",	0, &pico, NULL, _("Search prompting on"), _("Search prompting off"), _("  Search prompting ") },
	{"menu_jump",	0, &menu_jump, NULL, _("Jump into menu is on"), _("Jump into menu is off"), _("  Jump into menu ") },
	{"autoswap",	0, &autoswap, NULL, _("Autoswap ^KB and ^KK"), _("Autoswap off "), _("  Autoswap mode ") },
	{"indentc",	5, NULL, (unsigned char *) &fdefault.indentc, _("Indent char %d (SPACE=32, TAB=9, ^C to abort): "), 0, _("  Indent char "), 0, 0, 255 },
	{"istep",	5, NULL, (unsigned char *) &fdefault.istep, _("Indent step %d (^C to abort): "), 0, _("  Indent step "), 0, 1, 64 },
	{"french",	4, NULL, (unsigned char *) &fdefault.french, _("One space after periods for paragraph reformat"), _("Two spaces after periods for paragraph reformat"), _("  French spacing ") },
	{"flowed",	4, NULL, (unsigned char *) &fdefault.flowed, _("One space after paragraph line"), _("No spaces after paragraph lines"), _("  Flowed text ") },
	{"highlight",	4, NULL, (unsigned char *) &fdefault.highlight, _("Highlighting enabled"), _("Highlighting disabled"), _("H Highlighting ") },
	{"spaces",	4, NULL, (unsigned char *) &fdefault.spaces, _("Inserting spaces when tab key is hit"), _("Inserting tabs when tab key is hit"), _("  No tabs ") },
	{"mid",	0, &mid, NULL, _("Cursor will be recentered on scrolls"), _("Cursor will not be recentered on scroll"), _("C Center on scroll ") },
	{"guess_crlf",0, &guesscrlf, NULL, _("Automatically detect MS-DOS files"), _("Do not automatically detect MS-DOS files"), _("  Auto detect CR-LF ") },
	{"guess_indent",0, &guessindent, NULL, _("Automatically detect indentation"), _("Do not automatically detect indentation"), _("  Guess indent ") },
	{"guess_non_utf8",0, &guess_non_utf8, NULL, _("Automatically detect non-UTF-8 in UTF-8 locale"), _("Do not automatically detect non-UTF-8"), _("  Guess non-UTF-8 ") },
	{"guess_utf8",0, &guess_utf8, NULL, _("Automatically detect UTF-8 in non-UTF-8 locale"), _("Do not automatically detect UTF-8"), _("  Guess UTF-8 ") },
	{"transpose",0, &transpose, NULL, _("Menu is transposed"), _("Menus are not transposed"), _("  Transpose menus ") },
	{"crlf",	4, NULL, (unsigned char *) &fdefault.crlf, _("CR-LF is line terminator"), _("LF is line terminator"), _("Z CR-LF (MS-DOS) ") },
	{"linums",	4, NULL, (unsigned char *) &fdefault.linums, _("Line numbers enabled"), _("Line numbers disabled"), _("N Line numbers ") },
	{"marking",	0, &marking, NULL, _("Anchored block marking on"), _("Anchored block marking off"), _("  Marking ") },
	{"asis",	0, &dspasis, NULL, _("Characters above 127 shown as-is"), _("Characters above 127 shown in inverse"), _("  Meta chars as-is ") },
	{"force",	0, &force, NULL, _("Last line forced to have NL when file saved"), _("Last line not forced to have NL"), _("  Force last NL ") },
	{"joe_state",0, &joe_state, NULL, _("~/.joe_state file will be updated"), _("~/.joe_state file will not be updated"), _("  Joe_state file ") },
	{"nobackup",	4, NULL, (unsigned char *) &fdefault.nobackup, _("Nobackup enabled"), _("Nobackup disabled"), _("  No backup ") },
	{"nobackups",	0, &nobackups, NULL, _("Backup files will not be made"), _("Backup files will be made"), _("  Disable backups ") },
	{"nolocks",	0, &nolocks, NULL, _("Files will not be locked"), _("Files will be locked"), _("  Disable locks ") },
	{"nomodcheck",	0, &nomodcheck, NULL, _("No file modification time check"), _("File modification time checking enabled"), _("  Disable mtime check ") },
	{"nocurdir",	0, &nocurdir, NULL, _("No current dir"), _("Current dir enabled"), _("  Disable current dir ") },
	{"break_hardlinks",	0, &break_links, NULL, _("Hardlinks will be broken"), _("Hardlinks not broken"), _("  Break hard links ") },
	{"break_links",	0, &break_symlinks, NULL, _("Links will be broken"), _("Links not broken"), _("  Break links ") },
	{"lightoff",	0, &lightoff, NULL, _("Highlighting turned off after block operations"), _("Highlighting not turned off after block operations"), _("  Auto unmark ") },
	{"exask",	0, &exask, NULL, _("Prompt for filename in save & exit command"), _("Don't prompt for filename in save & exit command"), _("  Exit ask ") },
	{"beep",	0, &joe_beep, NULL, _("Warning bell enabled"), _("Warning bell disabled"), _("B Beeps ") },
	{"nosta",	0, &staen, NULL, _("Top-most status line disabled"), _("Top-most status line enabled"), _("  Disable status line ") },
	{"keepup",	0, &keepup, NULL, _("Status line updated constantly"), _("Status line updated once/sec"), _("  Fast status line ") },
	{"pg",		1, &pgamnt, NULL, _("Lines to keep for PgUp/PgDn or -1 for 1/2 window (%d): "), 0, _("  No. PgUp/PgDn lines "), 0, -1, 64 },
	{"undo_keep",		1, &undo_keep, NULL, _("No. undo records to keep, or (0 for infinite): "), 0, _("  No. undo records "), 0, -1, 64 },
	{"csmode",	0, &csmode, NULL, _("Start search after a search repeats previous search"), _("Start search always starts a new search"), _("  Continued search ") },
	{"rdonly",	4, NULL, (unsigned char *) &fdefault.readonly, _("Read only"), _("Full editing"), _("O Read only ") },
	{"smarthome",	4, NULL, (unsigned char *) &fdefault.smarthome, _("Smart home key enabled"), _("Smart home key disabled"), _("  Smart home key ") },
	{"indentfirst",	4, NULL, (unsigned char *) &fdefault.indentfirst, _("Smart home goes to indentation first"), _("Smart home goes home first"), _("  To indent first ") },
	{"smartbacks",	4, NULL, (unsigned char *) &fdefault.smartbacks, _("Smart backspace key enabled"), _("Smart backspace key disabled"), _("  Smart backspace ") },
	{"purify",	4, NULL, (unsigned char *) &fdefault.purify, _("Indentation clean up enabled"), _("Indentation clean up disabled"), _("  Clean up indents ") },
	{"picture",	4, NULL, (unsigned char *) &fdefault.picture, _("Picture drawing mode enabled"), _("Picture drawing mode disabled"), _("P Picture mode ") },
	{"backpath",	2, &backpath, NULL, _("Backup files stored in (%s): "), 0, _("  Path to backup files ") },
	{"syntax",	9, NULL, NULL, _("Select syntax (^C to abort): "), 0, _("Y Syntax") },
	{"encoding",13, NULL, NULL, _("Select file character set (^C to abort): "), 0, _("E Encoding ") },
	{"single_quoted",	4, NULL, (unsigned char *) &fdefault.single_quoted, _("Single quoting enabled"), _("Single quoting disabled"), _("  ^G ignores '... ' ") },
	{"no_double_quoted",4, NULL, (unsigned char *) &fdefault.no_double_quoted, _("Double quoting disabled"), _("Double quoting enabled"), _("  ^G ignores \"... \" ") },
	{"c_comment",	4, NULL, (unsigned char *) &fdefault.c_comment, _("/* comments enabled"), _("/* comments disabled"), _("  ^G ignores /*...*/ ") },
	{"cpp_comment",	4, NULL, (unsigned char *) &fdefault.cpp_comment, _("// comments enabled"), _("// comments disabled"), _("  ^G ignores //... ") },
	{"pound_comment",	4, NULL, (unsigned char *) &fdefault.pound_comment, _("# comments enabled"), _("# comments disabled"), _("  ^G ignores #... ") },
	{"vhdl_comment",	4, NULL, (unsigned char *) &fdefault.vhdl_comment, _("-- comments enabled"), _("-- comments disabled"), _("  ^G ignores --... ") },
	{"semi_comment",	4, NULL, (unsigned char *) &fdefault.semi_comment, _("; comments enabled"), _("; comments disabled"), _("  ^G ignores ;... ") },
	{"tex_comment",	4, NULL, (unsigned char *) &fdefault.tex_comment, _("% comments enabled"), _("% comments disabled"), _("  ^G ignores %... ") },
	{"text_delimiters",	6, NULL, (unsigned char *) &fdefault.text_delimiters, _("Text delimiters (%s): "), 0, _("  Text delimiters ") },
	{"language",	6, NULL, (unsigned char *) &fdefault.language, _("Language (%s): "), 0, _("V Language ") },
	{"cpara",		6, NULL, (unsigned char *) &fdefault.cpara, _("Characters which can indent paragraphs (%s): "), 0, _("  Paragraph indent chars ") },
	{"floatmouse",	0, &floatmouse, 0, _("Clicking can move the cursor past end of line"), _("Clicking past end of line moves cursor to the end"), _("  Click past end ") },
	{"rtbutton",	0, &rtbutton, 0, _("Mouse action is done with the right button"), _("Mouse action is done with the left button"), _("  Right button ") },
	{"nonotice",	0, &nonotice, NULL, 0, 0, 0 },
	{"help_is_utf8",	0, &help_is_utf8, NULL, 0, 0, 0 },
	{"noxon",	0, &noxon, NULL, 0, 0, 0 },
	{"orphan",	0, &orphan, NULL, 0, 0, 0 },
	{"help",	0, &help, NULL, 0, 0, 0 },
	{"dopadding",	0, &dopadding, NULL, 0, 0, 0 },
	{"lines",	1, &lines, NULL, 0, 0, 0, 0, 2, 1024 },
	{"columns",	1, &columns, NULL, 0, 0, 0, 0, 2, 1024 },
	{"skiptop",	1, &skiptop, NULL, 0, 0, 0, 0, 0, 64 },
	{"notite",	0, &notite, NULL, 0, 0, 0 },
	{"mouse",	0, &xmouse, NULL, 0, 0, 0 },
	{"usetabs",	0, &usetabs, NULL, 0, 0, 0 },
	{"assume_color", 0, &assume_color, NULL, 0, 0, 0 },
	{"assume_256color", 0, &assume_256color, NULL, 0, 0, 0 },
	{"joexterm", 0, &joexterm, NULL, 0, 0, 0 },
	{ NULL,		0, NULL, NULL, NULL, NULL, NULL, 0, 0, 0 }
};

/* Initialize .ofsts above.  Is this really necessary? */

int isiz = 0;
HASH *opt_tab;

static void izopts(void)
{
	int x;

	opt_tab = htmk(128);

	for (x = 0; glopts[x].name; ++x) {
		htadd(opt_tab, glopts[x].name, glopts + x);
		switch (glopts[x].type) {
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
			glopts[x].ofst = glopts[x].addr - (unsigned char *) &fdefault;
		}
	}
	isiz = 1;
}

/* Set a global or local option:
 * 's' is option name
 * 'arg' is a possible argument string (taken only if option has an arg)
 * 'options' points to options structure to modify (can be NULL).
 * 'set'==0: set only in 'options' if it's given.
 * 'set'!=0: set global variable option.
 * return value: no. of fields taken (1 or 2), or 0 if option not found.
 *
 * So this function is used both to set options, and to parse over options
 * without setting them.
 *
 * These combinations are used:
 *
 * glopt(name,arg,NULL,1): set global variable option
 * glopt(name,arg,NULL,0): parse over option
 * glopt(name,arg,options,0): set file local option
 * glopt(name,arg,&fdefault,1): set default file options
 * glopt(name,arg,options,1): set file local option
 */

int glopt(unsigned char *s, unsigned char *arg, OPTIONS *options, int set)
{
	int val;
	int ret = 0;
	int st = 1;	/* 1 to set option, 0 to clear it */
	struct glopts *opt;

	/* Initialize offsets */
	if (!isiz)
		izopts();

	/* Clear instead of set? */
	if (s[0] == '-') {
		st = 0;
		++s;
	}

	opt = htfind(opt_tab, s);

	if (opt) {
		switch (opt->type) {
		case 0: /* Global variable flag option */
			if (set)
				*(int *)opt->set = st;
			break;
		case 1: /* Global variable integer option */
			if (set && arg) {
				sscanf((char *)arg, "%d", &val);
				if (val >= opt->low && val <= opt->high)
					*(int *)opt->set = val;
			}
			break;
		case 2: /* Global variable string option */
			if (set) {
				if (arg)
					*(unsigned char **) opt->set = strdup(arg);
				else
					*(unsigned char **) opt->set = 0;
			}
			break;
		case 4: /* Local option flag */
			if (options)
				*(int *) ((unsigned char *) options + opt->ofst) = st;
			break;
		case 5: /* Local option integer */
			if (arg) {
				if (options) {
					sscanf((char *)arg, "%d", &val);
					if (val >= opt->low && val <= opt->high)
						*(int *) ((unsigned char *)
							  options + opt->ofst) = val;
				} 
			}
			break;
		case 6: /* Local string option */
			if (options) {
				if (arg) {
					*(unsigned char **) ((unsigned char *)
							  options + opt->ofst) = strdup(arg);
				} else {
					*(unsigned char **) ((unsigned char *)
							  options + opt->ofst) = 0;
				}
			}
			break;
		case 7: /* Local option numeric + 1, with range checking */
			if (arg) {
				int zz = 0;

				sscanf((char *)arg, "%d", &zz);
				if (zz >= opt->low && zz <= opt->high) {
					--zz;
					if (options)
						*(int *) ((unsigned char *)
							  options + opt->ofst) = zz;
				}
			}
			break;

		case 9: /* Set syntax */
			if (arg && options)
				options->syntax_name = strdup(arg);
			/* this was causing all syntax files to be loaded...
			if (arg && options)
				options->syntax = load_syntax(arg); */
			break;

		case 13: /* Set byte mode encoding */
			if (arg && options)
				options->map_name = strdup(arg);
			break;
		}
		/* This is a stupid hack... */
		if ((opt->type & 3) == 0 || !arg)
			return 1;
		else
			return 2;
	} else {
		/* Why no case 6, string option? */
		/* Keymap, mold, mnew, etc. are not strings */
		/* These options do not show up in ^T */
		if (!strcmp(s, "lmsg")) {
			if (arg) {
				if (options)
					options->lmsg = strdup(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "rmsg")) {
			if (arg) {
				if (options)
					options->rmsg = strdup(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "keymap")) {
			if (arg) {
				int y;

				for (y = 0; !joe_isspace(locale_map,arg[y]); ++y) ;
				if (!arg[y])
					arg[y] = 0;
				if (options && y)
					options->context = strdup(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "mnew")) {
			if (arg) {
				int sta;

				if (options)
					options->mnew = mparse(NULL, arg, &sta);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "mfirst")) {
			if (arg) {
				int sta;

				if (options)
					options->mfirst = mparse(NULL, arg, &sta);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "mold")) {
			if (arg) {
				int sta;

				if (options)
					options->mold = mparse(NULL, arg, &sta);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "msnew")) {
			if (arg) {
				int sta;

				if (options)
					options->msnew = mparse(NULL, arg, &sta);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "msold")) {
			if (arg) {
				int sta;

				if (options)
					options->msold = mparse(NULL, arg, &sta);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "text_color")) {
			if (arg) {
				bg_text = meta_color(arg);
				bg_help = bg_text;
				bg_prompt = bg_text;
				bg_menu = bg_text;
				bg_msg = bg_text;
				bg_stalin = bg_text;
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "help_color")) {
			if (arg) {
				bg_help = meta_color(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "status_color")) {
			if (arg) {
				bg_stalin = meta_color(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "menu_color")) {
			if (arg) {
				bg_menu = meta_color(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "prompt_color")) {
			if (arg) {
				bg_prompt = meta_color(arg);
				ret = 2;
			} else
				ret = 1;
		} else if (!strcmp(s, "msg_color")) {
			if (arg) {
				bg_msg = meta_color(arg);
				ret = 2;
			} else
				ret = 1;
		}
	}

	return ret;
}

/* Option setting user interface (^T command) */

static int doabrt1(BW *bw, int *xx)
{
	free(xx);
	return -1;
}

static int doopt1(BW *bw, unsigned char *s, int *xx, int *notify)
{
	int ret = 0;
	int x = *xx;
	int v;

	free(xx);
	switch (glopts[x].type) {
	case 1:
		v = calc(bw, s);
		if (merr) {
			msgnw(bw->parent, merr);
			ret = -1;
		} else if (v >= glopts[x].low && v <= glopts[x].high)
			*(int *)glopts[x].set = v;
		else {
			msgnw(bw->parent, joe_gettext(_("Value out of range")));
			ret = -1;
		}
		break;
	case 2:
		if (s[0])
			*(unsigned char **) glopts[x].set = strdup(s);
		break;
	case 6:
		*(unsigned char **)((unsigned char *)&bw->o+glopts[x].ofst) = strdup(s);
		break;
	case 5:
		v = calc(bw, s);
		if (merr) {
			msgnw(bw->parent, merr);
			ret = -1;
		} else if (v >= glopts[x].low && v <= glopts[x].high)
			*(int *) ((unsigned char *) &bw->o + glopts[x].ofst) = v;
		else {
			msgnw(bw->parent, joe_gettext(_("Value out of range")));
			ret = -1;
		}
		break;
	case 7:
		v = calc(bw, s) - 1.0;
		if (merr) {
			msgnw(bw->parent, merr);
			ret = -1;
		} else if (v >= glopts[x].low && v <= glopts[x].high)
			*(int *) ((unsigned char *) &bw->o + glopts[x].ofst) = v;
		else {
			msgnw(bw->parent, joe_gettext(_("Value out of range")));
			ret = -1;
		}
		break;
	}
	vsrm(s);
	bw->b->o = bw->o;
	wfit(bw->parent->t);
	updall();
	if (notify)
		*notify = 1;
	return ret;
}

static int dosyntax(BW *bw, unsigned char *s, int *xx, int *notify)
{
	int ret = 0;
	struct high_syntax *syn;

	syn = load_syntax(s);

	if (syn)
		bw->o.syntax = syn;
	else
		msgnw(bw->parent, joe_gettext(_("Syntax definition file not found")));

	vsrm(s);
	bw->b->o = bw->o;
	updall();
	if (notify)
		*notify = 1;
	return ret;
}

unsigned char **syntaxes = NULL; /* Array of available syntaxes */

static int syntaxcmplt(BW *bw)
{
	if (!syntaxes) {
		unsigned char *oldpwd = pwd();
		unsigned char **t;
		unsigned char *p;
		int x, y;

		if (chpwd((JOEDATA "syntax")))
			return -1;
		t = rexpnd("*.jsf");
		if (!t) {
			chpwd(oldpwd);
			return -1;
		}
		if (!aLEN(t)) {
			varm(t);
			chpwd(oldpwd);
			return -1;
		}

		for (x = 0; x != aLEN(t); ++x) {
			unsigned char *r = vsncpy(NULL,0,t[x],(unsigned char *)strrchr((char *)(t[x]),'.')-t[x]);
			syntaxes = vaadd(syntaxes,r);
		}
		varm(t);

		p = (unsigned char *)getenv("HOME");
		if (p) {
			unsigned char buf[1024];
			snprintf(buf,sizeof(buf),"%s/.joe/syntax",p);
			if (!chpwd(buf) && (t = rexpnd("*.jsf"))) {
				for (x = 0; x != aLEN(t); ++x)
					*strrchr((char *)t[x],'.') = 0;
				for (x = 0; x != aLEN(t); ++x) {
					for (y = 0; y != aLEN(syntaxes); ++y)
						if (!strcmp(t[x],syntaxes[y]))
							break;
					if (y == aLEN(syntaxes)) {
						unsigned char *r = vsncpy(NULL,0,sv(t[x]));
						syntaxes = vaadd(syntaxes,r);
					}
				}
				varm(t);
			}
		}

		vasort(av(syntaxes));
		chpwd(oldpwd);
	}
	return simple_cmplt(bw,syntaxes);
}

int check_for_hex(BW *bw)
{
	W *w;
	if (bw->o.hex)
		return 1;
	for (w = bw->parent->link.next; w != bw->parent; w = w->link.next)
		if ((w->watom == &watomtw || w->watom == &watompw) && ((BW *)w->object)->b == bw->b &&
		    ((BW *)w->object)->o.hex)
		    	return 1;
	return 0;
}

static int doencoding(BW *bw, unsigned char *s, int *xx, int *notify)
{
	int ret = 0;
	struct charmap *map;


	map = find_charmap(s);

	if (map && map->type && check_for_hex(bw)) {
		msgnw(bw->parent, joe_gettext(_("UTF-8 encoding not allowed with hexadecimal windows")));
		if (notify)
			*notify = 1;
		return -1;
	}

	if (map) {
		bw->o.charmap = map;
		snprintf(msgbuf, JOE_MSGBUFSIZE, joe_gettext(_("%s encoding assumed for this file")), map->name);
		msgnw(bw->parent, msgbuf);
	} else
		msgnw(bw->parent, joe_gettext(_("Character set not found")));

	vsrm(s);
	bw->b->o = bw->o;
	updall();
	if (notify)
		*notify = 1;
	return ret;
}

unsigned char **encodings = NULL; /* Array of available encodinges */

static int encodingcmplt(BW *bw)
{
	if (!encodings) {
		encodings = get_encodings();
		vasort(av(encodings));
	}
	return simple_cmplt(bw,encodings);
}

/* Menus of macros */

struct rc_menu_entry {
	MACRO *m;
	unsigned char *name;
};

struct rc_menu {
	struct rc_menu *next;	/* Next one in list */
	unsigned char *name;	/* Name of this menu */
	int last_position;	/* Last cursor position */
	int size;		/* Number of entries */
	struct rc_menu_entry **entries;
} *menus;

struct menu_instance {
	struct rc_menu *menu;
	unsigned char **s;
};

int find_option(unsigned char *s)
{
	int y;
	for (y = 0; glopts[y].name; ++y)
		if (!strcmp(glopts[y].name, s))
			return y;
	return -1;
}

struct rc_menu *find_menu(unsigned char *s)
{
	struct rc_menu *m;
	for (m = menus; m; m = m->next)
		if (!strcmp(m->name, s))
			break;
	return m;
}

struct rc_menu *create_menu(unsigned char *name)
{
	struct rc_menu *menu = find_menu(name);
	if (menu)
		return menu;
	menu = (struct rc_menu *)joe_malloc(sizeof(struct rc_menu));
	menu->name = strdup(name);
	menu->next = menus;
	menus = menu;
	menu->last_position = 0;
	menu->size = 0;
	menu->entries = 0;
	return menu;
}

void add_menu_entry(struct rc_menu *menu, unsigned char *entry_name, MACRO *m)
{
	struct rc_menu_entry *e = (struct rc_menu_entry *)joe_malloc(sizeof(struct rc_menu_entry));
	e->m = m;
	e->name = strdup(entry_name);
	++menu->size;
	if (!menu->entries) {
		menu->entries = (struct rc_menu_entry **)joe_malloc(menu->size * sizeof(struct rc_menu_entry *));
	} else {
		menu->entries = (struct rc_menu_entry **)joe_realloc(menu->entries, menu->size * sizeof(struct rc_menu_entry *));
	}
	menu->entries[menu->size - 1] = e;
}

static int olddoopt(BW *bw, int y, int flg, int *notify)
{
	int *xx;
	unsigned char buf[OPT_BUF_SIZE];

	if (y >= 0) {
		switch (glopts[y].type) {
		case 0:
			if (!flg)
				*(int *)glopts[y].set = !*(int *)glopts[y].set;
			else if (flg == 1)
				*(int *)glopts[y].set = 1;
			else
				*(int *)glopts[y].set = 0;
			msgnw(bw->parent, *(int *)glopts[y].set ? joe_gettext(glopts[y].yes) : joe_gettext(glopts[y].no));
			break;
		case 4:
			if (!flg)
				*(int *) ((unsigned char *) &bw->o + glopts[y].ofst) = !*(int *) ((unsigned char *) &bw->o + glopts[y].ofst);
			else if (flg == 1)
				*(int *) ((unsigned char *) &bw->o + glopts[y].ofst) = 1;
			else
				*(int *) ((unsigned char *) &bw->o + glopts[y].ofst) = 0;
			msgnw(bw->parent, *(int *) ((unsigned char *) &bw->o + glopts[y].ofst) ? joe_gettext(glopts[y].yes) : joe_gettext(glopts[y].no));
			if (glopts[y].ofst == (unsigned char *) &fdefault.readonly - (unsigned char *) &fdefault)
				bw->b->rdonly = bw->o.readonly;
			/* Kill UTF-8 mode if we switch to hex display */
			if (glopts[y].ofst == (unsigned char *) &fdefault.hex - (unsigned char *) &fdefault &&
			    bw->o.hex &&
			    bw->b->o.charmap->type) {
				doencoding(bw, vsncpy(NULL, 0, sc("C")), NULL, NULL);
			}
			break;
		case 6:
			xx = (int *) joe_malloc(sizeof(int));
			*xx = y;
			if(*(unsigned char **)((unsigned char *)&bw->o+glopts[y].ofst))
				snprintf(buf, OPT_BUF_SIZE, glopts[y].yes,*(unsigned char **)((unsigned char *)&bw->o+glopts[y].ofst));
			else
				snprintf(buf, OPT_BUF_SIZE, glopts[y].yes,"");
			if(wmkpw(bw->parent, buf, NULL, doopt1, NULL, doabrt1, utypebw, xx, notify, locale_map, 0))
				return 0;
			else
				return -1;
			/* break; warns on some systems */
		case 1:
			snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), *(int *)glopts[y].set);
			xx = (int *) joe_malloc(sizeof(int));

			*xx = y;
			if (wmkpw(bw->parent, buf, NULL, doopt1, NULL, doabrt1, utypebw, xx, notify, locale_map, 0))
				return 0;
			else
				return -1;
		case 2:
			if (*(unsigned char **) glopts[y].set)
				snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), *(unsigned char **) glopts[y].set);
			else
				snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), "");
			xx = (int *) joe_malloc(sizeof(int));

			*xx = y;
			if (wmkpw(bw->parent, buf, NULL, doopt1, NULL, doabrt1, utypebw, xx, notify, locale_map, 0))
				return 0;
			else
				return -1;
		case 5:
			snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), *(int *) ((unsigned char *) &bw->o + glopts[y].ofst));
			goto in;
		case 7:
			snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), *(int *) ((unsigned char *) &bw->o + glopts[y].ofst) + 1);
		      in:xx = (int *) joe_malloc(sizeof(int));

			*xx = y;
			if (wmkpw(bw->parent, buf, NULL, doopt1, NULL, doabrt1, utypebw, xx, notify, locale_map, 0))
				return 0;
			else
				return -1;

		case 9:
			snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), "");
			if (wmkpw(bw->parent, buf, NULL, dosyntax, NULL, NULL, syntaxcmplt, NULL, notify, locale_map, 0))
				return 0;
			else
				return -1;

		case 13:
			snprintf(buf, OPT_BUF_SIZE, joe_gettext(glopts[y].yes), "");
			if (wmkpw(bw->parent, buf, NULL, doencoding, NULL, NULL, encodingcmplt, NULL, notify, locale_map, 0))
				return 0;
			else
				return -1;
		}
	}
	if (notify)
		*notify = 1;
	bw->b->o = bw->o;
	wfit(bw->parent->t);
	updall();
	return 0;
}

static int doabrt(MENU *m, int x, struct menu_instance *mi)
{
	mi->menu->last_position = x;
	for (x = 0; mi->s[x]; ++x)
		vsrm(mi->s[x]);
	free(mi->s);
	free(mi);
	return -1;
}

int menu_flg; /* Key used to select menu entry */

static int execmenu(MENU *m, int x, struct menu_instance *mi, int flg)
{
	struct rc_menu *menu = mi->menu;
	int *notify = m->parent->notify;
	if (notify)
		*notify = 1;
	wabort(m->parent);
	menu_flg = flg;
	return exmacro(menu->entries[x]->m, 1);
}

int display_menu(BW *bw, struct rc_menu *menu, int *notify)
{
	struct menu_instance *m = (struct menu_instance *)joe_malloc(sizeof(struct menu_instance));
	unsigned char **s = (unsigned char **)joe_malloc(sizeof(unsigned char *) * (menu->size + 1));
	int x;
	for (x = 0; x != menu->size; ++x) {
		s[x] = stagen(NULL, bw, menu->entries[x]->name, ' ');
	}
	s[x] = 0;
	m->menu = menu;
	m->s = s;
	if (mkmenu(bw->parent, bw->parent, m->s, execmenu, doabrt, NULL, menu->last_position, m, notify))
		return 0;
	else
		return -1;
}

unsigned char *get_status(BW *bw, unsigned char *s)
{
	static unsigned char buf[OPT_BUF_SIZE];
	int y = find_option(s);
	if (y == -1)
		return "???";
	else {
		switch (glopts[y].type) {
			case 0: {
				return *(int *)glopts[y].set ? "ON" : "OFF";
			} case 1: {
				snprintf(buf, OPT_BUF_SIZE, "%d", *(int *)glopts[y].set);
				return buf;
			} case 4: {
				return *(int *) ((unsigned char *) &bw->o + glopts[y].ofst) ? "ON" : "OFF";
			} case 5: {
				snprintf(buf, OPT_BUF_SIZE, "%d", *(int *) ((unsigned char *) &bw->o + glopts[y].ofst));
				return buf;
			} case 7: {
				snprintf(buf, OPT_BUF_SIZE, "%d", *(int *) ((unsigned char *) &bw->o + glopts[y].ofst) + 1);
				return buf;
			} default: {
				return "";
			}
		}
	}
}

/* ^T command */

unsigned char **getmenus(void)
{
	unsigned char **s = vaensure(NULL, 20);
	struct rc_menu *m;

	for (m = menus; m; m = m->next)
		s = vaadd(s, vsncpy(NULL, 0, sz(m->name)));
	vasort(s, aLen(s));
	return s;
}

unsigned char **smenus = NULL;	/* Array of command names */

static int menucmplt(BW *bw)
{
	if (!smenus)
		smenus = getmenus();
	return simple_cmplt(bw,smenus);
}

static int domenu(BW *bw, unsigned char *s, void *object, int *notify)
{
	struct rc_menu *menu = find_menu(s);
	vsrm(s);
	if (!menu) {
		msgnw(bw->parent, joe_gettext(_("No such menu")));
		if (notify)
			*notify = 1;
		return -1;
	} else {
		bw->b->o.readonly = bw->o.readonly = bw->b->rdonly;
		return display_menu(bw, menu, notify);
	}
}

B *menuhist = NULL;

int umenu(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Menu: ")), &menuhist, domenu, "menu", NULL, menucmplt, NULL, NULL, locale_map, 0)) {
		return 0;
	} else {
		return -1;
	}
}

/* Simplified mode command */

unsigned char **getoptions(void)
{
	unsigned char **s = vaensure(NULL, 20);
	int x;

	for (x = 0; glopts[x].name; ++x)
		s = vaadd(s, vsncpy(NULL, 0, sz(glopts[x].name)));
	vasort(s, aLen(s));
	return s;
}

/* Command line */

unsigned char **sopts = NULL;	/* Array of command names */

static int optcmplt(BW *bw)
{
	if (!sopts)
		sopts = getoptions();
	return simple_cmplt(bw,sopts);
}

static int doopt(BW *bw, unsigned char *s, void *object, int *notify)
{
	int y = find_option(s);
	vsrm(s);
	if (y == -1) {
		msgnw(bw->parent, joe_gettext(_("No such option")));
		if (notify)
			*notify = 1;
		return -1;
	} else {
		int flg = menu_flg;
		menu_flg = 0;
		return olddoopt(bw, y, flg, notify);
	}
}

B *opthist = NULL;

int umode(BW *bw)
{
	if (wmkpw(bw->parent, joe_gettext(_("Option: ")), &opthist, doopt, "opt", NULL, optcmplt, NULL, NULL, locale_map, 0)) {
		return 0;
	} else {
		return -1;
	}
}

/* Process rc file
 * Returns 0 if the rc file was succefully processed
 *        -1 if the rc file couldn't be opened
 *         1 if there was a syntax error in the file
 */

int procrc(CAP *cap, unsigned char *name)
{
	OPTIONS *o = &fdefault;	/* Current options */
	KMAP *context = NULL;	/* Current context */
	struct rc_menu *current_menu = NULL;
	unsigned char buf[1024];	/* Input buffer */
	JFILE *fd;		/* rc file */
	int line = 0;		/* Line number */
	int err = 0;		/* Set to 1 if there was a syntax error */

	strncpy((char *)buf, (char *)name, sizeof(buf) - 1);
	buf[sizeof(buf)-1] = '\0';
	fd = jfopen(buf, "r");

	if (!fd)
		return -1;	/* Return if we couldn't open the rc file */

	fprintf(stderr,(char *)joe_gettext(_("Processing '%s'...")), name);
	fflush(stderr);

	while (jfgets(buf, sizeof(buf), fd)) {
		line++;
		switch (buf[0]) {
		case ' ':
		case '\t':
		case '\n':
		case '\f':
		case 0:
			break;	/* Skip comment lines */

		case '=':	/* Define a global color */
			{ /* # introduces comment */
			parse_color_def(&global_colors,buf+1,name,line);
			}
			break;

		case '*':	/* Select file types for file-type dependant options */
			{ /* Space and tab introduce comments- which means we can't have them in the regex */
				int x;

				o = (OPTIONS *) joe_malloc(sizeof(OPTIONS));
				*o = fdefault;
				for (x = 0; buf[x] && buf[x] != '\n' && buf[x] != ' ' && buf[x] != '\t'; ++x) ;
				buf[x] = 0;
				o->next = options;
				options = o;
				o->name_regex = strdup(buf);
			}
			break;
		case '+':	/* Set file contents match regex */
			{ /* No comments allowed- entire line used. */
				int x;

				for (x = 0; buf[x] && buf[x] != '\n' && buf[x] != '\r'; ++x) ;
				buf[x] = 0;
				if (o)
					o->contents_regex = strdup(buf+1);
			}
			break;
		case '-':	/* Set an option */
			{ /* parse option and arg.  arg goes to end of line.  This is bad. */
				unsigned char *opt = buf + 1;
				int x;
				unsigned char *arg = NULL;

				for (x = 0; buf[x] && buf[x] != '\n' && buf[x] != ' ' && buf[x] != '\t'; ++x) ;
				if (buf[x] && buf[x] != '\n') {
					buf[x] = 0;
					for (arg = buf + ++x; buf[x] && buf[x] != '\n'; ++x) ;
				}
				buf[x] = 0;
				if (!glopt(opt, arg, o, 2)) {
					err = 1;
					fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Unknown option %s")), name, line, opt);
				}
			}
			break;
		case '{':	/* Process help text.  No comment allowed after {name */
			{	/* everything after } is ignored. */
				line = help_init(fd,buf,line);
			}
			break;
		case ':':	/* Select context */
			{
				int x, c;

				for (x = 1; !joe_isspace_eof(locale_map,buf[x]); ++x) ;
				c = buf[x];
				buf[x] = 0;
				if (x != 1)
					if (!strcmp(buf + 1, "def")) {
						int y;

						for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
						for (y = x; !joe_isspace_eof(locale_map,buf[y]); ++y) ;
						c = buf[y];
						buf[y] = 0;
						if (y != x) {
							int sta;
							MACRO *m;

							if (joe_isblank(locale_map,c)
							    && (m = mparse(NULL, buf + y + 1, &sta)))
								addcmd(buf + x, m);
							else {
								err = 1;
								fprintf(stderr, (char *)joe_gettext(_("\n%s %d: macro missing from :def")), name, line);
							}
						} else {
							err = 1;
							fprintf(stderr, (char *)joe_gettext(_("\n%s %d: command name missing from :def")), name, line);
						}
					} else if (!strcmp(buf + 1, "inherit")) {
						if (context) {
							for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
							for (c = x; !joe_isspace_eof(locale_map,buf[c]); ++c) ;
							buf[c] = 0;
							if (c != x)
								kcpy(context, kmap_getcontext(buf + x));
							else {
								err = 1;
								fprintf(stderr, (char *)joe_gettext(_("\n%s %d: context name missing from :inherit")), name, line);
							}
						} else {
							err = 1;
							fprintf(stderr, (char *)joe_gettext(_("\n%s %d: No context selected for :inherit")), name, line);
						}
					} else if (!strcmp(buf + 1, "include")) {
						for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
						for (c = x; !joe_isspace_eof(locale_map,buf[c]); ++c) ;
						buf[c] = 0;
						if (c != x) {
							unsigned char bf[1024];
							unsigned char *p = (unsigned char *)getenv("HOME");
							int rtn = -1;
							bf[0] = 0;
							if (p && buf[x] != '/') {
								snprintf(bf,sizeof(bf),"%s/.joe/%s",p,buf + x);
								rtn = procrc(cap, bf);
							}
							if (rtn == -1 && buf[x] != '/') {
								snprintf(bf,sizeof(bf),"%s%s",JOERC,buf + x);
								rtn = procrc(cap, bf);
							}
							if (rtn == -1 && buf[x] == '/') {
								snprintf(bf,sizeof(bf),"%s",buf + x);
								rtn = procrc(cap, bf);
							}
							switch (rtn) {
							case 1:
								err = 1;
								break;
							case -1:
								fprintf(stderr, (char *)joe_gettext(_("\n%s %d: Couldn't open %s")), name, line, bf);
								err = 1;
								break;
							}
							context = 0;
							o = &fdefault;
						} else {
							err = 1;
							fprintf(stderr, (char *)joe_gettext(_("\n%s %d: :include missing file name")), name, line);
						}
					} else if (!strcmp(buf + 1, "delete")) {
						if (context) {
							int y;

							for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
							for (y = x; buf[y] != 0 && buf[y] != '\t' && buf[y] != '\n' && (buf[y] != ' ' || buf[y + 1]
															!= ' '); ++y) ;
							buf[y] = 0;
							kdel(context, buf + x);
						} else {
							err = 1;
							fprintf(stderr, (char *)joe_gettext(_("\n%s %d: No context selected for :delete")), name, line);
						}
					} else if (!strcmp(buf + 1, "defmap")) {
						for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
						for (c = x; !joe_isspace_eof(locale_map,buf[c]); ++c) ;
						buf[c] = 0;
						if (c != x) {
							context = kmap_getcontext(buf + x);
							current_menu = 0;
						} else {
							err = 1;
							fprintf(stderr, (char *)joe_gettext(_("\n%s %d: :defmap missing name")), name, line);
						}
					} else if (!strcmp(buf + 1, "defmenu")) {
						for (buf[x] = c; joe_isblank(locale_map,buf[x]); ++x) ;
						for (c = x; !joe_isspace_eof(locale_map,buf[c]); ++c) ;
						buf[c] = 0;
						current_menu = create_menu(buf + x);
						context = 0;
					} else {
						context = kmap_getcontext(buf + 1);
						current_menu = 0;
					}
				else {
					err = 1;
					fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Invalid context name")), name, line);
				}
			}
			break;
		default:	/* Get key-sequence to macro binding */
			{
				int x, y;
				MACRO *m;

				if (!context && !current_menu) {
					err = 1;
					fprintf(stderr,(char *)joe_gettext(_("\n%s %d: No context selected for macro to key-sequence binding")), name, line);
					break;
				}

				m = 0;
			      macroloop:
				m = mparse(m, buf, &x);
				if (x == -1) {
					err = 1;
					fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Unknown command in macro")), name, line);
					break;
				} else if (x == -2) {
					jfgets(buf, 1024, fd);
					++line;
					goto macroloop;
				}
				if (!m)
					break;

				/* Skip to end of key sequence */
				for (y = x; buf[y] != 0 && buf[y] != '\t' && buf[y] != '\n' && (buf[y] != ' ' || buf[y + 1] != ' '); ++y) ;
				buf[y] = 0;

				if (current_menu) {
					/* Add menu entry */
					add_menu_entry(current_menu, buf + x, m);
				} else {
					/* Add binding to context */
					if (kadd(cap, context, buf + x, m) == -1) {
						fprintf(stderr,(char *)joe_gettext(_("\n%s %d: Bad key sequence '%s'")), name, line, buf + x);
						err = 1;
					}
				}
			}
			break;
		}
	}
	jfclose(fd);		/* Close rc file */

	/* Print proper ending string */
	if (err)
		fprintf(stderr, (char *)joe_gettext(_("\ndone\n")));
	else
		fprintf(stderr, (char *)joe_gettext(_("done\n")));

	return err;		/* 0 for success, 1 for syntax error */
}

/* Save a history buffer */

void save_hist(FILE *f,B *b)
{
	unsigned char buf[512];
	int len;
	if (b) {
		P *p = pdup(b->bof, "save_hist");
		P *q = pdup(b->bof, "save_hist");
		if (b->eof->line>10)
			pline(p,b->eof->line-10);
		pset(q,p);
		while (!piseof(p)) {
			pnextl(q);
			if (q->byte-p->byte<512) {
				len = q->byte - p->byte;
				brmem(p,buf,len);
			} else {
				brmem(p,buf,512);
				len = 512;
			}
			fprintf(f,"\t");
			emit_string(f,buf,len);
			fprintf(f,"\n");
			pset(p,q);
		}
		prm(p);
		prm(q);
	}
	fprintf(f,"done\n");
}

/* Load a history buffer */

void load_hist(FILE *f,B **bp)
{
	B *b;
	unsigned char buf[1024];
	unsigned char bf[1024];
	P *q;

	b = *bp;
	if (!b)
		*bp = b = bmk(NULL);

	q = pdup(b->eof, "load_hist");

	while(fgets((char *)buf,1023,f) && strcmp(buf,"done\n")) {
		unsigned char *p = buf;
		int len;
		parse_ws(&p,'#');
		len = parse_string(&p,bf,sizeof(bf));
		if (len>0) {
			binsm(q,bf,len);
			pset(q,b->eof);
		}
	}

	prm(q);
}

/* Save state */

#define STATE_ID (unsigned char *)"# JOE state file v1.0\n"

void save_state()
{
	unsigned char *home = (unsigned char *)getenv("HOME");
	int old_mask;
	FILE *f;
	if (!joe_state)
		return;
	if (!home)
		return;
	snprintf(stdbuf,stdsiz,"%s/.joe_state",home);
	old_mask = umask(0066);
	f = fopen((char *)stdbuf,"w");
	umask(old_mask);
	if(!f)
		return;

	/* Write ID */
	fprintf(f,"%s",(char *)STATE_ID);

	/* Write state information */
	fprintf(f,"search\n"); save_srch(f);
	fprintf(f,"macros\n"); save_macros(f);
	fprintf(f,"files\n"); save_hist(f,filehist);
	fprintf(f,"find\n"); save_hist(f,findhist);
	fprintf(f,"replace\n"); save_hist(f,replhist);
	fprintf(f,"run\n"); save_hist(f,runhist);
	fprintf(f,"build\n"); save_hist(f,buildhist);
	fprintf(f,"grep\n"); save_hist(f,grephist);
	fprintf(f,"cmd\n"); save_hist(f,cmdhist);
	fprintf(f,"math\n"); save_hist(f,mathhist);
	fprintf(f,"yank\n"); save_yank(f);
	fprintf(f,"file_pos\n"); save_file_pos(f);
	fclose(f);
}

/* Load state */

void load_state()
{
	unsigned char *home = (unsigned char *)getenv("HOME");
	unsigned char buf[1024];
	FILE *f;
	if (!joe_state)
		return;
	if (!home)
		return;
	snprintf(stdbuf,stdsiz,"%s/.joe_state",home);
	f = fopen((char *)stdbuf,"r");
	if(!f)
		return;

	/* Only read state information if the version is correct */
	if (fgets((char *)buf,1024,f) && !strcmp(buf,STATE_ID)) {

		/* Read state information */
		while(fgets((char *)buf,1023,f)) {
			if(!strcmp(buf,"search\n"))
				load_srch(f);
			else if(!strcmp(buf,"macros\n"))
				load_macros(f);
			else if(!strcmp(buf,"files\n"))
				load_hist(f,&filehist);
			else if(!strcmp(buf,"find\n"))
				load_hist(f,&findhist);
			else if(!strcmp(buf,"replace\n"))
				load_hist(f,&replhist);
			else if(!strcmp(buf,"run\n"))
				load_hist(f,&runhist);
			else if(!strcmp(buf,"build\n"))
				load_hist(f,&buildhist);
			else if(!strcmp(buf,"grep\n"))
				load_hist(f,&grephist);
			else if(!strcmp(buf,"cmd\n"))
				load_hist(f,&cmdhist);
			else if(!strcmp(buf,"math\n"))
				load_hist(f,&mathhist);
			else if(!strcmp(buf,"yank\n"))
				load_yank(f);
			else if (!strcmp(buf,"file_pos\n"))
				load_file_pos(f);
			else { /* Unknown... skip until next done */
				while(fgets((char *)buf,1023,f) && strcmp(buf,"done\n"));
			}
		}
	}

	fclose(f);
}
