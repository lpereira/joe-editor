/*
 *	Various utilities
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *		(C) 2001 Marek 'Marx' Grac
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

/*
 * return minimum/maximum of two numbers
 */
unsigned int uns_min(unsigned int a, unsigned int b)
{
	return a < b ? a : b;
}

signed int int_min(signed int a, signed int b)
{
	return a < b ? a : b;
}

signed long int long_max(signed long int a, signed long int b)
{
	return a > b ? a : b;
}

signed long int long_min(signed long int a, signed long int b)
{
	return a < b ? a : b;
}

/* Versions of 'read' and 'write' which automatically retry when interrupted */
ssize_t joe_read(int fd, void *buf, size_t size)
{
	ssize_t rt;

	do {
		rt = read(fd, buf, size);
	} while (rt < 0 && errno == EINTR);
	return rt;
}

ssize_t joe_write(int fd, void *buf, size_t size)
{
	ssize_t rt;

	do {
		rt = write(fd, buf, size);
	} while (rt < 0 && errno == EINTR);
	return rt;
}

int joe_ioctl(int fd, int req, void *ptr)
{
	int rt;
	do {
		rt = ioctl(fd, req, ptr);
	} while (rt == -1 && errno == EINTR);
	return rt;
}

void *joe_malloc(size_t size)
{
	void *p = malloc(size);
	if (!p)
		ttsig(-1);
	return p;
}

void *joe_calloc(size_t nmemb,size_t size)
{
	void *p = calloc(nmemb, size);
	if (!p)
		ttsig(-1);
	return p;
}

void *joe_realloc(void *ptr,size_t size)
{
	void *p = realloc(ptr, size);
	if (!p)
		ttsig(-1);
	return p;
}

void joe_free(void *ptr)
{
	free(ptr);
}

unsigned char *zstr(unsigned char *a, unsigned char *b)
{
	return (unsigned char *)strstr((char *)a,(char *)b);
}

unsigned char *zncpy(unsigned char *a, unsigned char *b, size_t len)
{
	strncpy((char *)a,(char *)b,len);
	return a;
}

unsigned char *zcat(unsigned char *a, unsigned char *b)
{
	strcat((char *)a,(char *)b);
	return a;
}

unsigned char *zchr(unsigned char *s, int c)
{
	return (unsigned char *)strchr((char *)s,c);
}

unsigned char *zrchr(unsigned char *s, int c)
{
	return (unsigned char *)strrchr((char *)s,c);
}

/* Zstrings */

void rm_zs(ZS z)
{
	joe_free(z.s);
}

ZS raw_mk_zs(GC **gc,unsigned char *s,int len)
{
	ZS zs;
	zs.s = (unsigned char *)joe_malloc(len+1);
	if (len)
		memcpy(zs.s,s,len);
	zs.s[len] = 0;
	return zs;
}

#ifndef SIG_ERR
#define SIG_ERR ((sighandler_t) -1)
#endif

/* wrapper to hide signal interface differrencies */
int joe_set_signal(int signum, sighandler_t handler)
{
	int retval;
#ifdef HAVE_SIGACTION
	struct sigaction sact;

	memset(&sact, 0, sizeof(sact));
	sact.sa_handler = handler;
#ifdef SA_INTERRUPT
	sact.sa_flags = SA_INTERRUPT;
#endif
	retval = sigaction(signum, &sact, NULL);
#elif defined(HAVE_SIGVEC)
	struct sigvec svec;

	memset(&svec, 0, sizeof(svec));
	svec.sv_handler = handler;
#ifdef HAVE_SV_INTERRUPT
	svec.sv_flags = SV_INTERRUPT;
#endif
	retval = sigvec(signum, &svec, NULL);
#else
	retval = (signal(signum, handler) != SIG_ERR) ? 0 : -1;
#ifdef HAVE_SIGINTERRUPT
	siginterrupt(signum, 1);
#endif
#endif
	return(retval);
}

/* Helpful little parsing utilities */

/* Skip whitespace and return first non-whitespace character */

int parse_ws(unsigned char **pp,int cmt)
{
	unsigned char *p = *pp;
	while (*p==' ' || *p=='\t')
		++p;
	if (*p=='\r' || *p=='\n' || *p==cmt)
		*p = 0;
	*pp = p;
	return *p;
}

/* Parse an identifier into a buffer.  Identifier is truncated to a maximum of len-1 chars. */

int parse_ident(unsigned char **pp, unsigned char *buf, int len)
{
	unsigned char *p = *pp;
	if (joe_isalpha_(locale_map,*p)) {
		while(len > 1 && joe_isalnum_(locale_map,*p))
			*buf++= *p++, --len;
		*buf=0;
		while(joe_isalnum_(locale_map,*p))
			++p;
		*pp = p;
		return 0;
	} else
		return -1;
}

/* Parse to next whitespace */

int parse_tows(unsigned char **pp, unsigned char *buf)
{
	unsigned char *p = *pp;
	while (*p && *p!=' ' && *p!='\t' && *p!='\n' && *p!='\r' && *p!='#')
		*buf++ = *p++;

	*pp = p;
	*buf = 0;
	return 0;
}

/* Parse over a specific keyword */

int parse_kw(unsigned char **pp, unsigned char *kw)
{
	unsigned char *p = *pp;
	while(*kw && *kw==*p)
		++kw, ++p;
	if(!*kw && !joe_isalnum_(locale_map,*p)) {
		*pp = p;
		return 0;
	} else
		return -1;
}

/* Parse a field (same as parse_kw, but string must be terminated with whitespace) */

int parse_field(unsigned char **pp, unsigned char *kw)
{
	unsigned char *p = *pp;
	while(*kw && *kw==*p)
		++kw, ++p;
	if(!*kw && (!*p || *p==' ' || *p=='\t' || *p=='#' || *p=='\n' || *p=='\r')) {
		*pp = p;
		return 0;
	} else
		return -1;
}

/* Parse a specific character */

int parse_char(unsigned char **pp, unsigned char c)
{
	unsigned char *p = *pp;
	if (*p == c) {
		*pp = p+1;
		return 0;
	} else
		return -1;
}

/* Parse an integer.  Returns 0 for success. */

int parse_int(unsigned char **pp, int *buf)
{
	unsigned char *p = *pp;
	if ((*p>='0' && *p<='9') || *p=='-') {
		*buf = atoi((char *)p);
		if(*p=='-')
			++p;
		while(*p>='0' && *p<='9')
			++p;
		*pp = p;
		return 0;
	} else
		return -1;
}

/* Parse a long */

int parse_long(unsigned char **pp, long *buf)
{
	unsigned char *p = *pp;
	if ((*p>='0' && *p<='9') || *p=='-') {
		*buf = atol((char *)p);
		if(*p=='-')
			++p;
		while(*p>='0' && *p<='9')
			++p;
		*pp = p;
		return 0;
	} else
		return -1;
}

/* Parse a string of the form "xxxxx" into a fixed-length buffer.  The
 * address of the buffer is 'buf'.  The length of this buffer is 'len'.  A
 * terminating NUL is added to the parsed string.  If the string is larger
 * than the buffer, the string is truncated.
 *
 * C string escape sequences are handled.
 *
 * 'p' holds an address of the input string pointer.  The pointer
 * is updated to point right after the parsed string if the function
 * succeeds.
 *
 * Returns the length of the string (not including the added NUL), or
 * -1 if there is no string or if the input ended before the terminating ".
 */

int parse_string(unsigned char **pp, unsigned char *buf, int len)
{
	unsigned char *start = buf;
	unsigned char *p= *pp;
	if(*p=='\"') {
		++p;
		while(len > 1 && *p && *p!='\"') {
			int x = 50;
			int c = escape(0, &p, &x);
			*buf++ = c;
			--len;
		}
		*buf = 0;
		while(*p && *p!='\"')
			if(*p=='\\' && p[1])
				p += 2;
			else
				p++;
		if(*p == '\"') {
			*pp = p + 1;
			return buf - start;
		}
	}
	return -1;
}

/* Emit a string */

void emit_string(FILE *f,unsigned char *s,int len)
{
	fputc('"',f);
	while(len) {
		if (*s=='"' || *s=='\\')
			fputc('\\',f), fputc(*s,f);
		else if(*s=='\n')
			fputc('\\',f), fputc('n',f);
		else if(*s=='\r')
			fputc('\\',f), fputc('r',f);
		else if(*s==0)
			fputc('\\',f), fputc('0',f), fputc('0',f), fputc('0',f);
		else
			fputc(*s,f);
		++s;
		--len;
	}
	fputc('"',f);
}

/* Parse a character range: a-z */

int parse_range(unsigned char **pp, int *first, int *second)
{
	unsigned char *p= *pp;
	int a, b;
	if(!*p)
		return -1;
	if(*p=='\\' && p[1]) {
		++p;
		if(*p=='n')
			a = '\n';
		else if(*p=='t')
  			a = '\t';
		else
			a = *p;
		++p;
	} else
		a = *p++;
	if(*p=='-' && p[1]) {
		++p;
		if(*p=='\\' && p[1]) {
			++p;
			if(*p=='n')
				b = '\n';
			else if(*p=='t')
				b = '\t';
			else
				b = *p;
			++p;
		} else
			b = *p++;
	} else
		b = a;
	*first = a;
	*second = b;
	*pp = p;
	return 0;
}
