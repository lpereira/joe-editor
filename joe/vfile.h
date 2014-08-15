/*
 *	Software virtual memory system
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#ifndef _JOE_VFILE_H
#define _JOE_VFILE_H 1

/* Page header */

struct vpage {
	VPAGE	*next;		/* Next page with same hash value */
	VFILE	*vfile;		/* Owner vfile */
	off_t	addr;		/* Address of this page */
	int	count;		/* Reference count */
	int	dirty;		/* Set if page changed */
	unsigned char	*data;		/* The data in the page */
};

/* File structure */

struct vfile {
	LINK(VFILE)	link;	/* Doubly linked list of vfiles */
	off_t	size;		/* Number of bytes in physical file */
	off_t	alloc;		/* Number of bytes allocated to file */
	int	fd;		/* Physical file */
	int	writeable;	/* Set if we can write */
	unsigned char	*name;		/* File name.  0 if unnamed */
	int	flags;		/* Set if this is only a temporary file */

	/* For array I/O */
	unsigned char	*vpage1;	/* Page address */
	long	addr;		/* File address of above page */

	/* For stream I/O */
	unsigned char	*bufp;		/* Buffer pointer */
	unsigned char	*vpage;		/* Buffer pointer points in here */
	int	left;		/* Space left in bufp */
	int	lv;		/* Amount of append space at end of buffer */
};
/* Additions:
 *
 * Should we remove size checking from rc()?  Would make it faster...
 *
 * Should be able to open more than one stream on a file so that vseek
 * doesn't have to get called so much when more than one user is involed
 *
 * Also should have dupopen call to make more streams for a file
 *
 * Make vputs faster
 *
 * Should have a version which will use memory mapped files, if they exist
 * in the os.
 *
 * Would be nice if we could transparantly open non-file streams and pipes.
 * Should there be an buffering option for that?  So we can seek on pipes to
 * get previously read data?
 */

extern unsigned char *vbase;		/* Data first entry in vheader refers to */
extern VPAGE **vheaders;	/* Array of headers */

/* VFILE *vtmp(V);
 *
 * Open a temporary virtual file.  File goes away when closed.  No actual
 * file is generated if everything fits in memory.
 */
VFILE *vtmp(void);

/* off_t vsize(VFILE *);
 *
 * Return size of file
 */

#define vsize(vfile) \
	( \
	  (vfile)->left<(vfile)->lv ? \
	    (vfile)->alloc+(vfile)->lv-(vfile)->left \
	  : \
	    (vfile)->alloc \
	)

/* void vclose(VFILE *vfile);
 *
 * Close a file.
 */
void vclose(VFILE *vfile);

/* void vflsh(void); 
 *
 * Write all changed pages to the disk
 */

void vflsh(void);

/* void vflshf(VFILE *vfile);
 *
 * Write changed pages for a specific file to the disk
 */

void vflshf(VFILE *vfile);

/* char *vlock(VFILE *vfile,off_t addr);
 *
 * Translate virtual address to physical address.  'addr' does not have
 * to be on any particular alignment, but if you wish to access more than
 * a single byte, you have to be aware of where page boundaries are (virtual
 * address multiples of PGSIZE).
 *
 * The page containing the data is locked in memory (so that it won't be
 * freed or used for something else) until 'vunlock' is used.
 *
 * Warning:  If you allocate more than one page and use (change) them out of
 * order, vflsh will screw up if writing past the end of a file is illegal
 * in the host filesystem.
 *
 * Also:  This function does not allocate space to the file.  Use valloc()
 * for that.  You can vlock() pages past the allocated size of the file, but
 * be careful when you do this (you normally shouldn't- the only time you
 * ever might want to is to implement your own version of valloc()).
 */

unsigned char *vlock(VFILE *vfile, off_t addr);

/* VPAGE *vheader(char *);
 * Return address of page header for given page
 */

#define vheader(p) (vheaders[(physical((unsigned char *)(p))-physical(vbase))>>LPGSIZE])

/* void vchanged(char *);
 *
 * Indicate that a vpage was changed so that it will be written back to the
 * file.  Any physical address which falls within the page may be given.
 */

#define vchanged(vpage) ( vheader(vpage)->dirty=1 )

/* void vunlock(char *);
 * Unreference a vpage (call one vunlock for every vlock)
 * Any physical address which falls within the page may be given.
 */

#define vunlock(vpage)  ( --vheader(vpage)->count )

/* void vupcount(char *);
 * Indicate that another reference is being made to a vpage
 */

#define vupcount(vpage) ( ++vheader(vpage)->count )

/* long valloc(VFILE *vfile,long size);
 *
 * Allocate space at end of file
 *
 * Returns file address of beginning of allocated space
 */

off_t my_valloc(VFILE *vfile, off_t size);

#endif
