/*
 *	UNIX Tty and Process interface
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
#include "types.h"

/* Needed for TIOCGWINSZ detection below */
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <pty.h>
#include <utmp.h>
#include <termios.h>
#include <sys/time.h>

int idleout = 1;

/* Global configuration variables */

int noxon = 0;			/* Set if ^S/^Q processing should be disabled */

/* The terminal */

static FILE *termin = NULL;
static FILE *termout = NULL;

/* Original state of tty */

struct termios oldterm;

/* Output buffer, index and size */
static unsigned char obuf[4096];
static int obufp = 0;

/* Input buffer */

static int have = 0;		/* Set if we have pending input */
static unsigned char havec;	/* Character read in during pending input check */
int leave = 0;			/* When set, typeahead checking is disabled */

/* TTY mode flag.  1 for open, 0 for closed */
static int ttymode = 0;

/* Signal state flag.  1 for joe, 0 for normal */
static int ttysig = 0;

/* Stuff for shell windows */

static pid_t kbdpid;		/* PID of kbd client */
static int ackkbd = -1;		/* Editor acks keyboard client to this */

static int mpxfd;		/* Editor reads packets from this fd */
static int mpxsfd;		/* Clients send packets to this fd */

static int nmpx = 0;
static int acceptch = NO_MORE_DATA;	/* =-1 if we have last packet */

struct packet {
	MPX *who;
	int size;
	int ch;
	unsigned char data[1024];
} pack;

MPX asyncs[NPROC];

/* Set signals for JOE */
void sigjoe(void)
{
	if (ttysig)
		return;
	ttysig = 1;
	joe_set_signal(SIGHUP, ttsig);
	joe_set_signal(SIGTERM, ttsig);
	joe_set_signal(SIGABRT, ttsig);
	joe_set_signal(SIGINT, SIG_IGN);
	joe_set_signal(SIGPIPE, SIG_IGN);
}

/* Restore signals for exiting */
void signrm(void)
{
	if (!ttysig)
		return;
	ttysig = 0;
	joe_set_signal(SIGABRT, SIG_DFL);
	joe_set_signal(SIGHUP, SIG_DFL);
	joe_set_signal(SIGTERM, SIG_DFL);
	joe_set_signal(SIGINT, SIG_DFL);
	joe_set_signal(SIGPIPE, SIG_DFL);
}

/* Open terminal and set signals */

void ttopen(void)
{
	sigjoe();
	ttopnn();
}

/* Close terminal and restore signals */

void ttclose(void)
{
	ttclsn();
	signrm();
}

int tthave(void)
{
        return have;
}

static int winched = 0;
int ticked = 0;

/* Window size interrupt handler */
static RETSIGTYPE winchd(int unused)
{
	++winched;
	ticked = 1;
	REINSTALL_SIGHANDLER(SIGWINCH, winchd);
}

/* Second ticker */

static RETSIGTYPE dotick(int unused)
{
	ticked = 1;
}

void tickoff(void)
{
	struct itimerval val;
	val.it_value.tv_sec = 0;
	val.it_value.tv_usec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	setitimer(ITIMER_REAL,&val,NULL);
}

void tickon(void)
{
	struct itimerval val;
	struct timeval now;
	gettimeofday(&now, NULL);
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_usec = 0;
	if (auto_scroll) {
		time_t now = mnow();
		time_t tim = (now > auto_trig_time) ? 1 : auto_trig_time - now;
		tim *= 1000;
		val.it_value.tv_sec = 0;
		val.it_value.tv_usec = tim;
	} else {
		val.it_value.tv_sec = 60 - (now.tv_sec % 60);
		val.it_value.tv_usec = 1000000 - now.tv_usec;
	}
	ticked = 0;
	joe_set_signal(SIGALRM, dotick);
	setitimer(ITIMER_REAL,&val,NULL);
}

static int open_terminal_handles(void)
{
        if (termin && termout)
                return 1;

        if (idleout) {
                termin = stdin;
                termout = stdout;
        } else {
                termin = fopen("/dev/tty", "r");
                termout = fopen("/dev/tty", "w");
        }

        return termin && termout;
}

/* Open terminal */
void ttopnn(void)
{
	int x;
	struct termios newterm;

	if (!open_terminal_handles()) {
                fprintf(stderr, (char *)joe_gettext(_("Couldn\'t open /dev/tty\n")));
                exit(1);
	}

        joe_set_signal(SIGWINCH, winchd);

	if (ttymode)
		return;
	ttymode = 1;
	fflush(termout);

	tcgetattr(fileno(termin), &oldterm);
	newterm = oldterm;
	newterm.c_lflag = 0;
	if (noxon)
		newterm.c_iflag &= ~(ICRNL | IGNCR | INLCR | IXON | IXOFF);
	else
		newterm.c_iflag &= ~(ICRNL | IGNCR | INLCR);
	newterm.c_oflag = 0;
	newterm.c_cc[VMIN] = 1;
	newterm.c_cc[VTIME] = 0;
	tcsetattr(fileno(termin), TCSADRAIN, &newterm);
}

/* Close terminal */

void ttclsn(void)
{
	int oleave;

	if (ttymode)
		ttymode = 0;
	else
		return;

	oleave = leave;
	leave = 1;

	ttflsh();

	tcsetattr(fileno(termin), TCSADRAIN, &oldterm);
	leave = oleave;
}

/* FLush output and check for typeahead */

int ttflsh(void)
{
	/* Flush output */
	if (obufp) {
	        joe_write(fileno(termout), obuf, obufp);
		obufp = 0;
	}

	/* Ack previous packet */
	if (ackkbd != -1 && acceptch != NO_MORE_DATA && !have) {
		unsigned char c = 0;

		if (pack.who && pack.who->func)
			joe_write(pack.who->ackfd, &c, 1);
		else
			joe_write(ackkbd, &c, 1);
		acceptch = NO_MORE_DATA;
	}

	/* Check for typeahead or next packet */

	if (!have && !leave) {
		if (ackkbd != -1) {
			fcntl(mpxfd, F_SETFL, O_NDELAY);
			if (read(mpxfd, &pack, sizeof(struct packet) - 1024) > 0) {
				fcntl(mpxfd, F_SETFL, 0);
				joe_read(mpxfd, pack.data, pack.size);
				have = 1;
				acceptch = pack.ch;
			} else
				fcntl(mpxfd, F_SETFL, 0);
		} else {
			/* Set terminal input to non-blocking */
			fcntl(fileno(termin), F_SETFL, O_NDELAY);

			/* Try to read */
			if (read(fileno(termin), &havec, 1) == 1)
				have = 1;

			/* Set terminal back to blocking */
			fcntl(fileno(termin), F_SETFL, 0);
		}
	}
	return 0;
}

/* Read next character from input */

void mpxdied(MPX *m);

static time_t last_time;

int ttgetc(void)
{
	int stat;
	time_t new_time;
	int flg;


	tickon();

      loop:
      	flg = 0;
      	/* Status line clock */
	new_time = time(NULL);
	if (new_time != last_time) {
		last_time = new_time;
		dostaupd = 1;
		ticked = 1;
	}
	/* Autoscroller */
	if (auto_scroll && mnow() >= auto_trig_time) {
		do_auto_scroll();
		ticked = 1;
		flg = 1;
	}
	ttflsh();
	while (winched) {
		winched = 0;
		dostaupd = 1;
		edupd(1);
		ttflsh();
	}
	if (ticked) {
		edupd(flg);
		ttflsh();
		tickon();
	}
	if (ackkbd != -1) {
		if (!have) {	/* Wait for input */
			stat = read(mpxfd, &pack, sizeof(struct packet) - 1024);

			if (pack.size && stat > 0) {
				joe_read(mpxfd, pack.data, pack.size);
			} else if (stat < 1) {
				if (winched || ticked)
					goto loop;
				else
					ttsig(0);
			}
			acceptch = pack.ch;
		}
		have = 0;
		if (pack.who) {	/* Got bknd input */
			if (acceptch != NO_MORE_DATA) {
				if (pack.who->func) {
					pack.who->func(pack.who->object, pack.data, pack.size);
					edupd(1);
				}
			} else
				mpxdied(pack.who);
			goto loop;
		} else {
			if (acceptch != NO_MORE_DATA) {
				tickoff();
				return acceptch;
			}
			else {
				tickoff();
				ttsig(0);
				return 0;
			}
		}
	}
	if (have) {
		have = 0;
	} else {
		if (read(fileno(termin), &havec, 1) < 1) {
			if (winched || ticked)
				goto loop;
			else
				ttsig(0);
		}
	}
	tickoff();
	return havec;
}

/* Write string to output */

void ttputs(unsigned char *s)
{
	while (*s)
	        ttputc(*s++);
}

/* Get window size */

void ttgtsz(int *x, int *y)
{
#ifdef TIOCGSIZE
	struct ttysize getit;
#else
#ifdef TIOCGWINSZ
	struct winsize getit;
#endif
#endif

	*x = 0;
	*y = 0;

#ifdef TIOCGSIZE
	if (joe_ioctl(fileno(termout), TIOCGSIZE, &getit) != -1) {
		*x = getit.ts_cols;
		*y = getit.ts_lines;
	}
#else
#ifdef TIOCGWINSZ
	if (joe_ioctl(fileno(termout), TIOCGWINSZ, &getit) != -1) {
		*x = getit.ws_col;
		*y = getit.ws_row;
	}
#endif
#endif
}

int ttshell(unsigned char *cmd)
{
	int x, omode = ttymode;
	int stat= -1;
	unsigned char *s = (unsigned char *)getenv("SHELL");

	if (!s) {
		s = "/bin/sh";
		/* return; */
	}
	ttclsn();
	if ((x = fork()) != 0) {
		if (x != -1)
			wait(&stat);
		if (omode)
			ttopnn();
		return stat;
	} else {
		signrm();
		if (cmd)
			execl((char *)s, (char *)s, "-c", cmd, NULL);
		else {
			fprintf(stderr, (char *)joe_gettext(_("You are at the command shell.  Type 'exit' to return\n")));
			execl((char *)s, (char *)s, NULL);
		}
		_exit(0);
		return 0;
	}
}

/* Create keyboard task */

static void mpxresume(void)
{
	int fds[2];
	pipe(fds);
	acceptch = NO_MORE_DATA;
	have = 0;
	if (!(kbdpid = fork())) {
		close(fds[1]);
		do {
			unsigned char c;
			int sta;

			pack.who = 0;
			sta = joe_read(fileno(termin), &c, 1);
			if (sta == 0)
				pack.ch = NO_MORE_DATA;
			else
				pack.ch = c;
			pack.size = 0;
			joe_write(mpxsfd, &pack, sizeof(struct packet) - 1024);
		} while (joe_read(fds[0], &pack, 1) == 1);
		_exit(0);
	}
	close(fds[0]);
	ackkbd = fds[1];
}

/* Kill keyboard task */

static void mpxsusp(void)
{
	if (ackkbd!=-1) {
		kill(kbdpid, 9);
		while (wait(NULL) < 0 && errno == EINTR)
			/* do nothing */;
		close(ackkbd);
	}
}

/* We used to leave the keyboard copy task around during suspend, but
   Cygwin gets confused when two processes are waiting for input and you
   change the tty from raw to cooked (on the call to ttopnn()): the keyboard
   process was stuck in cooked until he got a carriage return- then he
   switched back to raw (he's supposed to switch to raw without waiting for
   the end of line). Probably this should be done for ttshell() as well. */

void ttsusp(void)
{
	int omode;

	omode = ttymode;
	mpxsusp();
	ttclsn();
	fprintf(stderr, (char *)joe_gettext(_("You have suspended the program.  Type 'fg' to return\n")));
	kill(0, SIGTSTP);
	if (omode)
		ttopnn();
	if (ackkbd!= -1)
		mpxresume();
}

/* Stuff for asynchronous I/O multiplexing.  We do not use streams or
   select() because joe needs to work on versions of UNIX which predate
   these calls.  Instead, when there is multiple async sources, we use
   helper processes which packetize data from the sources.  A header on each
   packet indicates the source.  There is no guarentee that packets getting
   written to the same pipe don't get interleaved, but you can reasonable
   rely on it with small packets. */

static void mpxstart(void)
{
	int fds[2];
	pipe(fds);
	mpxfd = fds[0];
	mpxsfd = fds[1];
	mpxresume();
}

static void mpxend(void)
{
	mpxsusp();
	ackkbd = -1;
	close(mpxfd);
	close(mpxsfd);
	if (have)
		havec = pack.ch;
}

/* Get a pty/tty pair.  Returns open pty in 'ptyfd' and returns tty name
 * string in static buffer or NULL if couldn't get a pair.
 */

static unsigned char *getpty(int *ptyfd)
{
	static unsigned char name[32];
	int ttyfd;

        if (openpty(ptyfd, &ttyfd, (char *)name, NULL, NULL) == 0)
           return(name);
        else
	   return (NULL);
}

/* Shell dies signal handler.  Puts pty in non-block mode so
 * that read returns with <1 when all data from process has
 * been read. */
static int dead = 0;
static int death_fd;
static RETSIGTYPE death(int unused)
{
	fcntl(death_fd,F_SETFL,O_NDELAY);
	wait(NULL);
	dead = 1;
}

/* Build a new environment, but replace one variable */

static unsigned char **newenv(unsigned char **old, unsigned char *s)
{
	unsigned char **new;
	int x, y, z;

	for (x = 0; old[x]; ++x) ;
	new = (unsigned char **) joe_malloc((x + 2) * sizeof(unsigned char *));

	for (x = 0, y = 0; old[x]; ++x) {
		for (z = 0; s[z] != '='; ++z)
			if (s[z] != old[x][z])
				break;
		if (s[z] == '=') {
			if (s[z + 1])
				new[y++] = s;
		} else
			new[y++] = old[x];
	}
	if (x == y)
		new[y++] = s;
	new[y] = 0;
	return new;
}

/* Create a shell process */

/* If out_only is set, leave program's stdin attached to JOE's stdin */

MPX *mpxmk(int *ptyfd, unsigned char *cmd, unsigned char **args, void (*func) (/* ??? */), void *object, void (*die) (/* ??? */), void *dieobj, int out_only)
{
	unsigned char buf[80];
	int fds[2];
	int comm[2];
	pid_t pid;
	int x;
	MPX *m = 0;
	unsigned char *name;

	/* Get pty/tty pair */
	if (!(name = getpty(ptyfd)))
		return NULL;

	/* Find free slot */
	for (x = 0; x != NPROC; ++x)
		if (!asyncs[x].func) {
			m = asyncs + x;
			break;
		}
	if (x==NPROC)
		return NULL;

	/* Fixes cygwin console bug: if you fork() with inverse video he assumes you want
	 * ESC [ 0 m to keep it in inverse video from then on. */
	set_attr(maint->t,0);

	/* Flush output */
	ttflsh();

	/* Bump no. current async inputs to joe */
	++nmpx;

	/* Start input multiplexer */
	if (ackkbd == -1)
		mpxstart();

	/* Remember callback function */
	m->func = func;
	m->object = object;
	m->die = die;
	m->dieobj = dieobj;

	/* Acknowledgement pipe */
	pipe(fds);
	m->ackfd = fds[1];

	/* PID number pipe */
	pipe(comm);


	/* Create processes... */
	if (!(m->kpid = fork())) {
		/* This process copies data from shell to joe */
		/* After each packet it sends to joe it waits for
		   an acknowledgement from joe so that it can not get
		   too far ahead with buffering */

		/* Close joe side of pipes */
		close(fds[1]);
		close(comm[0]);

		/* Flag which indicates child died */
		dead = 0;
		death_fd = *ptyfd;
		joe_set_signal(SIGCHLD, death);

		if (!(pid = fork())) {
			/* This process becomes the shell */
			signrm();

			/* Close pty (we only need tty) */
			close(*ptyfd);

			/* All of this stuff is for disassociating ourself from
			   controlling tty (session leader) and starting a new
			   session.  This is the most non-portable part of UNIX- second
			   only to pty/tty pair creation. */

			/* Close all fds */
			for (x = (out_only ? 1 : 0); x != 32; ++x)
				close(x);	/* Yes, this is quite a kludge... all in the
						   name of portability */

			/* Open the TTY */
			if ((x = open((char *)name, O_RDWR)) != -1) {	/* Standard input */
				unsigned char **env = newenv(mainenv, "TERM=");

				if (!out_only) {
					login_tty(x);

					tcsetattr(0, TCSADRAIN, &oldterm);
					/* We could probably have a special TTY set-up for JOE, but for now
					 * we'll just use the TTY setup for the TTY was was run on */

					/* Execute the shell */
					execve((char *)cmd, (char **)args, (char **)env);

					/* If shell didn't execute */
					snprintf(buf,sizeof(buf),joe_gettext(_("Couldn't execute shell '%s'\n")),cmd);
					write(1,(char *)buf,strlen(buf));
					sleep(1);

				} else {
					unsigned char buf[1024];
					int len;
					dup(x); /* Standard error */
					
					for (;;) {
						len = read(0, buf, sizeof(buf));
						if (len > 0)
							write(1, buf, len);
						else
							break;
					}
				}


			}

			_exit(0);
		}

		/* Tell JOE PID of shell */
		joe_write(comm[1], &pid, sizeof(pid));

		/* sigpipe should be ignored here. */

		/* This process copies data from shell to JOE until EOF.  It creates a packet
		   for each data */


		/* We don't really get EOF from a pty- it would just wait forever
		   until someone else writes to the tty.  So: when the shell
		   dies, the child died signal handler death() puts pty in non-block
		   mode.  This allows us to read any remaining data- then
		   read returns 0 and we know we're done. */

	      loop:
		pack.who = m;
		pack.ch = 0;

		/* Read data from process */
		pack.size = joe_read(*ptyfd, pack.data, 1024);

		/* On SUNOS 5.8, the very first read from the pty returns 0 for some reason */
		if (!pack.size)
			pack.size = joe_read(*ptyfd, pack.data, 1024);

		if (pack.size > 0) {
			/* Send data to JOE, wait for ack */
			joe_write(mpxsfd, &pack, sizeof(struct packet) - 1024 + pack.size);

			joe_read(fds[0], &pack, 1);
			goto loop;
		} else {
			/* Shell died: return */
			pack.ch = NO_MORE_DATA;
			pack.size = 0;
			joe_write(mpxsfd, &pack, sizeof(struct packet) - 1024);

			_exit(0);
		}
	}
	joe_read(comm[0], &m->pid, sizeof(m->pid));

	/* We only need comm once */
	close(comm[0]);
	close(comm[1]);

	/* Close other side of copy process pipe */
	close(fds[0]);
	return m;
}

void mpxdied(MPX *m)
{
	if (!--nmpx)
		mpxend();
	while (wait(NULL) < 0 && errno == EINTR)
		/* do nothing */;
	if (m->die)
		m->die(m->dieobj);
	m->func = NULL;
	edupd(1);
}

void ttputc(unsigned char c) {
        obuf[obufp++] = c;
        if (obufp == sizeof(obuf))
                ttflsh();
}
