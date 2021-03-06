/*
 *	Software virtual memory system
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

static VFILE vfiles = { {&vfiles, &vfiles} };	/* Known vfiles */
static VPAGE *freepages = NULL;	/* Linked list of free pages */
static VPAGE *htab[HTSIZE];	/* Hash table of page headers */
static long curvalloc = 0;	/* Amount of memory in use */
static long maxvalloc = ILIMIT;	/* Maximum allowed */
unsigned char *vbase;			/* Data first entry in vheader refers to */
VPAGE **vheaders = NULL;	/* Array of header addresses */
static int vheadsz = 0;		/* No. entries allocated to vheaders */

void vflsh(void)
{
	VPAGE *vp;
	VPAGE *vlowest;
	off_t addr;
	off_t last;
	VFILE *vfile;
	int x;

	for (vfile = vfiles.link.next; vfile != &vfiles; vfile = vfile->link.next) {
		last = -1;
	      loop:
		addr = MAXOFF;
		vlowest = NULL;
		for (x = 0; x != HTSIZE; x++)
			for (vp = htab[x]; vp; vp = vp->next)
				if (vp->addr < addr && vp->addr > last && vp->vfile == vfile && (vp->addr >= vfile->size || (vp->dirty && !vp->count))) {
					addr = vp->addr;
					vlowest = vp;
				}
		if (vlowest) {
			if (!vfile->name)
				vfile->name = mktmp(NULL);
			if (!vfile->fd)
				vfile->fd = open((char *)(vfile->name), O_RDWR);
			if (vfile->fd < 0)
				ttsig(-2);
			lseek(vfile->fd, addr, 0);
			if (addr + PGSIZE > vsize(vfile)) {
				if (joe_write(vfile->fd, vlowest->data, (int) (vsize(vfile) - addr)) < 0)
					ttsig(-2);
				vfile->size = vsize(vfile);
			} else {
				if (joe_write(vfile->fd, vlowest->data, PGSIZE) < 0)
					ttsig(-2);
				if (addr + PGSIZE > vfile->size)
					vfile->size = addr + PGSIZE;
			}
			vlowest->dirty = 0;
			last = addr;
			goto loop;
		}
	}
}

void vflshf(VFILE *vfile)
{
	VPAGE *vp;
	VPAGE *vlowest;
	off_t addr;
	int x;

      loop:
	addr = MAXOFF;
	vlowest = NULL;
	for (x = 0; x != HTSIZE; x++)
		for (vp = htab[x]; vp; vp = vp->next)
			if (vp->addr < addr && vp->dirty && vp->vfile == vfile && !vp->count) {
				addr = vp->addr;
				vlowest = vp;
			}
	if (vlowest) {
		if (!vfile->name)
			vfile->name = mktmp(NULL);
		if (!vfile->fd) {
			vfile->fd = open((char *)(vfile->name), O_RDWR);
		}
		if (vfile->fd < 0)
			ttsig(-2);
		lseek(vfile->fd, addr, 0);
		if (addr + PGSIZE > vsize(vfile)) {
			if (joe_write(vfile->fd, vlowest->data, (int) (vsize(vfile) - addr)) < 0)
				ttsig(-2);
			vfile->size = vsize(vfile);
		} else {
			if (joe_write(vfile->fd, vlowest->data, PGSIZE) < 0)
				ttsig(-2);
			if (addr + PGSIZE > vfile->size)
				vfile->size = addr + PGSIZE;
		}
		vlowest->dirty = 0;
		goto loop;
	}
}

static unsigned char *mema(int align, int size)
{
	unsigned char *z = (unsigned char *) joe_malloc(align + size);

	return z + align - physical(z) % align;
}

unsigned char *vlock(VFILE *vfile, off_t addr)
{
	VPAGE *vp, *pp;
	int x, y;
	off_t ofst = (addr & (PGSIZE - 1));

	addr -= ofst;

	for (vp = htab[((addr >> LPGSIZE) + (unsigned long) vfile) & (HTSIZE - 1)]; vp; vp = vp->next)
		if (vp->vfile == vfile && vp->addr == addr) {
			++vp->count;
			return vp->data + ofst;
		}

	if (freepages) {
		vp = freepages;
		freepages = vp->next;
		goto gotit;
	}

	if (curvalloc + PGSIZE <= maxvalloc) {
		vp = (VPAGE *) joe_malloc(sizeof(VPAGE) * INC);
		if (vp) {
			vp->data = (unsigned char *) mema(PGSIZE, PGSIZE * INC);
			if (vp->data) {
				int q;

				curvalloc += PGSIZE * INC;
				if (!vheaders) {
					vheaders = (VPAGE **) joe_malloc((vheadsz = INC) * sizeof(VPAGE *));
					vbase = vp->data;
				} else if (physical(vp->data) < physical(vbase)) {
					VPAGE **t = vheaders;
					int amnt = (physical(vbase) - physical(vp->data)) >> LPGSIZE;

					vheaders = (VPAGE **) joe_malloc((amnt + vheadsz) * sizeof(VPAGE *));
					memmove(vheaders + amnt, t, vheadsz * sizeof(VPAGE *));
					vheadsz += amnt;
					vbase = vp->data;
					free(t);
				} else if (((physical(vp->data + PGSIZE * INC) - physical(vbase)) >> LPGSIZE) > vheadsz) {
					vheaders = (VPAGE **)
					    joe_realloc(vheaders, (vheadsz = (((physical(vp->data + PGSIZE * INC) - physical(vbase)) >> LPGSIZE))) * sizeof(VPAGE *));
				}
				for (q = 1; q != INC; ++q) {
					vp[q].next = freepages;
					freepages = vp + q;
					vp[q].data = vp->data + q * PGSIZE;
					vheader(vp->data + q * PGSIZE) = vp + q;
				}
				vheader(vp->data) = vp;
				goto gotit;
			}
			free(vp);
			vp = NULL;
		}
	}

	for (y = HTSIZE, x = (random() & (HTSIZE - 1)); y; x = ((x + 1) & (HTSIZE - 1)), --y)
		for (pp = (VPAGE *) (htab + x), vp = pp->next; vp; pp = vp, vp = vp->next)
			if (!vp->count && !vp->dirty) {
				pp->next = vp->next;
				goto gotit;
			}
	vflsh();
	for (y = HTSIZE, x = (random() & (HTSIZE - 1)); y; x = ((x + 1) & (HTSIZE - 1)), --y)
		for (pp = (VPAGE *) (htab + x), vp = pp->next; vp; pp = vp, vp = vp->next)
			if (!vp->count && !vp->dirty) {
				pp->next = vp->next;
				goto gotit;
			}
	write(2, (char *)sz(joe_gettext(_("vfile: out of memory\n"))));
	exit(1);

      gotit:
	vp->addr = addr;
	vp->vfile = vfile;
	vp->dirty = 0;
	vp->count = 1;
	vp->next = htab[((addr >> LPGSIZE) + (unsigned long)vfile) & (HTSIZE - 1)];
	htab[((addr >> LPGSIZE) + (unsigned long)vfile) & (HTSIZE - 1)] = vp;

	if (addr < vfile->size) {
		if (!vfile->fd) {
			vfile->fd = open((char *)(vfile->name), O_RDWR);
		}
		if (vfile->fd < 0)
			ttsig(-2);
		lseek(vfile->fd, addr, 0);
		if (addr + PGSIZE > vfile->size) {
			if (joe_read(vfile->fd, vp->data, (int) (vfile->size - addr)) < 0)
				ttsig(-2);
			memset(vp->data + vfile->size - addr, 0, PGSIZE - (int) (vfile->size - addr));
		} else
			if (joe_read(vfile->fd, vp->data, PGSIZE) < 0)
				ttsig(-2);
	} else
		memset(vp->data, 0, PGSIZE);

	return vp->data + ofst;
}

VFILE *vtmp(void)
{
	VFILE *new = (VFILE *) joe_malloc(sizeof(VFILE));

	new->fd = 0;
	new->name = NULL;
	new->alloc = 0;
	new->size = 0;
	new->left = 0;
	new->lv = 0;
	new->vpage = NULL;
	new->flags = 1;
	new->vpage1 = NULL;
	new->addr = -1;
	return enqueb_f(VFILE, link, &vfiles, new);
}

void vclose(VFILE *vfile)
{
	VPAGE *vp, *pp;
	int x;

	if (vfile->vpage)
		vunlock(vfile->vpage);
	if (vfile->vpage1)
		vunlock(vfile->vpage1);
	if (vfile->name) {
		if (vfile->flags)
			unlink((char *)vfile->name);
		else
			vflshf(vfile);
		vsrm(vfile->name);
	}
	if (vfile->fd)
		close(vfile->fd);
	free(deque_f(VFILE, link, vfile));
	for (x = 0; x != HTSIZE; x++)
		for (pp = (VPAGE *) (htab + x), vp = pp->next; vp;)
			if (vp->vfile == vfile) {
				pp->next = vp->next;
				vp->next = freepages;
				freepages = vp;
				vp = pp->next;
			} else {
				pp = vp;
				vp = vp->next;
			}
}

off_t my_valloc(VFILE *vfile, off_t size)
{
	off_t start = vsize(vfile);

	vfile->alloc = start + size;
	if (vfile->lv) {
		if (vheader(vfile->vpage)->addr + PGSIZE > vfile->alloc)
			vfile->lv = PGSIZE - (vfile->alloc - vheader(vfile->vpage)->addr);
		else
			vfile->lv = 0;
	}
	return start;
}

