/* 
 *	Directory and path functions
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifdef HAVE_PATHS_H
#  include <paths.h>	/* for _PATH_TMP */
#endif
#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#include <dirent.h>

/********************************************************************/
unsigned char *namprt(unsigned char *path)
{
	unsigned char *z;

	z = path + slen(path);
	while ((z != path) && (z[-1] != '/'))
		--z;
	return vsncpy(NULL, 0, sz(z));
}
/********************************************************************/
unsigned char *namepart(unsigned char *tmp, unsigned char *path)
{
	unsigned char *z;

	z = path + strlen(path);
	while ((z != path) && (z[-1] != '/'))
		--z;
	return strcpy(tmp, z);
}
/********************************************************************/
unsigned char *dirprt(unsigned char *path)
{
	unsigned char *b = path;
	unsigned char *z = path + slen(path);

	while ((z != b) && (z[-1] != '/'))
		--z;
	return vsncpy(NULL, 0, path, z - path);
}
/********************************************************************/
unsigned char *begprt(unsigned char *path)
{
	unsigned char *z = path + slen(path);
	int drv = 0;

	while ((z != path + drv) && (z[-1] == '/'))
		--z;
	if (z == path + drv)
		return vsncpy(NULL, 0, sz(path));
	else {
		while ((z != path + drv) && (z[-1] != '/'))
			--z;
		return vsncpy(NULL, 0, path, z - path);
	}
}
/********************************************************************/
unsigned char *endprt(unsigned char *path)
{
	unsigned char *z = path + slen(path);
	int drv = 0;

	while ((z != path + drv) && (z[-1] == '/'))
		--z;
	if (z == path + drv)
		return vsncpy(NULL, 0, sc(""));
	else {
		while (z != path + drv && z[-1] != '/')
			--z;
		return vsncpy(NULL, 0, sz(z));
	}
}
/********************************************************************/
int mkpath(unsigned char *path)
{
	unsigned char *s;

	if (path[0] == '/') {
		if (chddir("/"))
			return 1;
		s = path;
		goto in;
	}

	while (path[0]) {
		int c;

		for (s = path; (*s) && (*s != '/'); s++) ;
		c = *s;
		*s = 0;
		if (chddir((char *)path)) {
			if (mkdir((char *)path, 0777))
				return 1;
			if (chddir((char *)path))
				return 1;
		}
		*s = c;
	      in:
		while (*s == '/')
			++s;
		path = s;
	}
	return 0;
}
/********************************************************************/
/* Create a temporary file */
/********************************************************************/
unsigned char *mktmp(unsigned char *where)
{
	unsigned char *name;
	int fd;
	unsigned namesize;

	if (!where)
		where = (unsigned char *)getenv("TEMP");
	if (!where)
		where = USTR _PATH_TMP;

	namesize = strlen(where) + 16;
	name = vsmk(namesize);	/* [G.Ghibo'] we need to use vsmk() and not malloc() as
				   area returned by mktmp() is destroyed later with
				   vsrm(); */
	snprintf(name, namesize, "%s/joe.tmp.XXXXXX", where);
	if((fd = mkstemp((char *)name)) == -1)
		return NULL;	/* FIXME: vflsh() and vflshf() */
				/* expect mktmp() always succeed!!! */

	fchmod(fd, 0600);       /* Linux glibc 2.0 mkstemp() creates it with */
				/* 0666 mode --> change it to 0600, so nobody */
				/* else sees content of temporary file */
	close(fd);

	return name;
}
/********************************************************************/
int rmatch(unsigned char *a, unsigned char *b)
{
	int flag, inv, c;

	for (;;)
		switch (*a) {
		case '*':
			++a;
			do {
				if (rmatch(a, b))
					return 1;
			} while (*b++);
			return 0;
		case '[':
			++a;
			flag = 0;
			if (*a == '^') {
				++a;
				inv = 1;
			} else
				inv = 0;
			if (*a == ']')
				if (*b == *a++)
					flag = 1;
			while (*a && (c = *a++) != ']')
				if ((c == '-') && (a[-2] != '[') && (*a)) {
					if ((*b >= a[-2]) && (*b <= *a))
						flag = 1;
				} else if (*b == c)
					flag = 1;
			if ((!flag && !inv) || (flag && inv) || (!*b))
				return 0;
			++b;
			break;
		case '?':
			++a;
			if (!*b)
				return 0;
			++b;
			break;
		case 0:
			if (!*b)
				return 1;
			else
				return 0;
		default:
			if (*a++ != *b++)
				return 0;
		}
}
/********************************************************************/
int isreg(unsigned char *s)
{
	int x;

	for (x = 0; s[x]; ++x)
		if ((s[x] == '*') || (s[x] == '?') || (s[x] == '['))
			return 1;
	return 0;
}
/********************************************************************/
unsigned char **rexpnd(unsigned char *word)
{
	void *dir;
	unsigned char **lst = NULL;

	struct dirent *de;
	dir = opendir(".");
	if (dir) {
		while ((de = readdir(dir)) != NULL)
			if (strcmp(".", de->d_name))
				if (rmatch(word, (unsigned char *)de->d_name))
					lst = vaadd(lst, vsncpy(NULL, 0, sz((unsigned char *)de->d_name)));
		closedir(dir);
	}
	return lst;
}
/********************************************************************/
unsigned char **rexpnd_users(unsigned char *word)
{
	unsigned char **lst = NULL;
	struct passwd *pw;

	while((pw=getpwent()))
		if (rmatch(word+1, (unsigned char *)pw->pw_name)) {
			unsigned char *t = vsncpy(NULL,0,sc("~"));
			lst = vaadd(lst, vsncpy(sv(t),sz((unsigned char *)pw->pw_name)));
			}
	endpwent();

	return lst;
}
/********************************************************************/
int chpwd(unsigned char *path)
{
	if ((!path) || (!path[0]))
		return 0;
	return chdir((char *)path);
}

/* The pwd function */
unsigned char *pwd(void)
{
	static unsigned char buf[PATH_MAX];
	unsigned char	*ret;

	ret = (unsigned char *)getcwd((char *)buf, PATH_MAX - 1);
	buf[PATH_MAX - 1] = '\0';

	return ret;
}

/* Simplify prefix by using ~ */
/* Expects s to have trailing / */

unsigned char *simplify_prefix(unsigned char *s)
{
	unsigned char *t = (unsigned char *)getenv("HOME");
	unsigned char *n;

	/* If current directory is prefixed with home directory, use ~... */
	if (t && !strncmp((char *)s,(char *)t,strlen(t)) && (!s[strlen(t)] || s[strlen(t)]=='/')) {
		n = vsncpy(NULL,0,sc("~/"));
		/* If anything more than just the home directory, add it */
		if (s[strlen(t)]) {
			n = vsncpy(sv(n),s+strlen(t)+1,strlen(s+strlen(t)+1));
		}
	} else {
		n = vsncpy(NULL,0,sz(s));
	}
	return n;
}
