/*
 * Copyright (C) 1994 by Charles Sandmann (sandmann@clio.rice.edu)
 * 
 * This file is part of ED.
 * 
 * ED is free software; you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation.
 * 
 * ED is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with ED
 * (see the file COPYING).  If not, write to the Free Software Foundation, 675
 * Mass Ave, Cambridge, MA 02139, USA.
 */

/* This code is based on MPG and read_size code by Rush Record, plus my 
   additions for setting the terminal size without an external script using
   ioctl calls.  The ioctl calls may not be supported on all systems (Posix?)
   so may be disabled by defining NO_TERMSIZE, and continuing to use the
   script "set_size.tmp" which this program will optionally generate.  To
   build, do:
     cc set_size.c -o set_size
   and to use it:
     set_size                   # This will read terminal size and stty it
     set_size csh               # Read size, stty it, and write csh script
     set_size anythingelse      # Read size, stty it, write sh/ksh/bsh script
   and then                     # for a csh/tcsh
     source set_size.tmp
     stty rows $ROWS cols $COLS
   or                           # for sh/ksh/bsh/bash, etc
     . set_size.tmp;
     stty rows ${ROWS} cols ${COLS};
   good luck, I hope you find it useful.
 */
#include <stdio.h>
#include <stdlib.h>

#define BUFFER_SIZE 1000
#define MAXCHAN 3

static int uschan,themchan;

void term_error(string,term)
char *string,*term;
{
	printf("Unable to %s for terminal %s.\r\n",string,term);
	cleanup();
}

#ifdef VMS
#include <string.h>
#include <ssdef.h>
#include <iodef.h>
#include <ttdef.h>
#include <tt2def.h>

typedef struct mode_str *mode_ptr;
typedef struct mode_str
{
	unsigned char class,type;
	unsigned short buffersize;
	union
	{
		unsigned long characteristics;
		unsigned char page[4];
	} u;
	unsigned long extended;
} mode_node;

typedef struct term_str *term_ptr;
typedef struct term_str
{
	short chan;
	long remain,efn;
	char *buf,*next;
	char valid,async,eight,setmode,setrmode,setsmode;
	unsigned short speeds,newspeeds;
	mode_node old_mode;
} term_node;
		
static char exhblk_set=0;
static long ctrlmask = -1;
static term_node term[MAXCHAN];
static short iosb[4];
static struct {short length,class;char *string;} desc={0,0x010e,0};
static long terminators[MAXCHAN];
static struct {long nbytes,*terminators;} terminator_desc = {32,terminators};

int cleanup()
{
	exit(1);
}

void set_normal_virtual(chars,eight)
long *chars,eight;
{
	*chars |= TT$M_LOWER|TT$M_MECHTAB|TT$M_SCOPE|TT$M_MECHFORM|TT$M_HOSTSYNC|TT$M_TTSYNC|TT$M_NOBRDCST|TT$M_NOECHO;
	if(eight)
		*chars |= TT$M_EIGHTBIT;
	else
		*chars &= ~TT$M_EIGHTBIT;
	*chars &= ~(TT$M_ESCAPE|TT$M_WRAP);
}

void ttyfini(chan)
int chan;
{
	term_ptr t;
	
	t = term + chan;
	if(t->valid)
	{
		if(t->setmode)
		{
			SYS$QIOW(0,t->chan,IO$_SETMODE,iosb,0,0,&t->old_mode,12,
				t->speeds,0,0,0);
			t->setmode = 0;
		}
		SYS$DASSGN(t->chan);
		t->valid = 0;
	}
}

int cleanup_routine()
{
	int i;
	
	for(i = 0;i < MAXCHAN;i++)
		ttyfini(i);
	LIB$ENABLE_CTRL(&ctrlmask,0);
	return(1);
}

int ttyinit(termname,eightbit)
char *termname;
int eightbit;
{
	long status,newchan;
	mode_node new_mode;
	term_ptr t;
	static long *exhblk[4];

	for(newchan = 0;newchan < MAXCHAN;newchan++)
		if(!term[newchan].valid)
			break;
	if(newchan == MAXCHAN)
	{
		printf("Too many terminal ports have been assigned.\n");
		cleanup();
	}
	t = term + newchan;
	desc.string=termname;
	desc.length=strlen(termname);
	status=SYS$ASSIGN(&desc,&t->chan,0,0);
	if(!(status&1))
		term_error("assign channel",termname);
	t->remain=0;
	if(!(t->buf=(char *)malloc(BUFFER_SIZE*sizeof(char))))
		term_error("get buffer",termname);
	if(!(LIB$GET_EF(&t->efn)&1))
		term_error("get event flag",termname);
	if(ctrlmask < 0)
	{
		status = 0x02100000;
		LIB$DISABLE_CTRL(&status,&ctrlmask);
	}
	if(!exhblk_set)
	{
		exhblk[0] = (long *)0;
		exhblk[1] = (long *)cleanup_routine;
		exhblk[2] = (long *)0;
		exhblk[3] = (long *)exhblk + 3;
		if(!((status=SYS$DCLEXH(exhblk))&1))
			LIB$STOP(status);
		exhblk_set = 1;
	}
	SYS$QIOW(0,t->chan,IO$_SENSEMODE,iosb,0,0,&t->old_mode,12,0,0,0,0);
	t->speeds = iosb[1];
	t->newspeeds = t->speeds;
	new_mode = t->old_mode;
	set_normal_virtual(&new_mode.u.characteristics,eightbit);
	SYS$QIOW(0,t->chan,IO$_SETMODE,iosb,0,0,&new_mode,12,t->newspeeds,0,0,0);
	t->setmode = t->valid = 1;
	t->eight = eightbit;
	t->async = t->setrmode = t->setsmode = 0;
	return(newchan);
}

int ttysetsize(chan,rows,cols)
int chan;
int rows;
int cols;
{
	term_ptr t;
	
	t = term + chan;
	if(t->old_mode.class == 66)	/* Terminal */
	{
		t->old_mode.u.page[3] = rows;
		t->old_mode.buffersize = cols;
	}
	return 0;
}

void ttynput(chan,c,n)
int chan;
char *c;
int n;
{
	long status;
	
	if(n)
		status = SYS$QIOW(0,term[chan].chan,IO$_WRITEVBLK|IO$M_NOFORMAT,iosb,0,0,c,n,0,0,0,0);
}

int ttyput(chan,c)
int chan;
char *c;
{
	int l;
	
	ttynput(chan,c,l = strlen(c));
	return(l);
}

int tty_timed_raw(chan,c,sec)
int chan;
char *c;
int sec;
{
	if(!term[chan].remain)
	{
		SYS$QIOW(0,term[chan].chan,IO$_TTYREADALL|IO$M_NOFILTR|IO$M_TIMED,
			iosb,0,0,term[chan].buf,BUFFER_SIZE-3,0,&terminator_desc,0,0);
		if(!(term[chan].remain=iosb[1]))
		{
			SYS$QIOW(0,term[chan].chan,IO$_TTYREADALL|IO$M_NOFILTR|IO$M_TIMED,iosb,0,0,term[chan].buf,1,sec,0,0,0);
			if(iosb[0] == SS$_TIMEOUT)
				return(0);
			term[chan].remain = 1;
		}
		term[chan].next=term[chan].buf;
	}
	*c = *term[chan].next++;
	if(!term[chan].eight)
		*c &= 0x7f;
	term[chan].remain--;
	return(1);
}

#else

#include <fcntl.h>
#include <signal.h>
#ifdef NeXT
#include <sgtty.h>
struct sgttyb savetty[MAXCHAN];
#else
#include <termios.h>
#ifndef NO_TERMSIZE
#ifndef sparc
#include <sys/ioctl.h>				/* Only for terminal size IOCTL */
#endif
#endif
struct termios savetty[MAXCHAN];
#endif
#include <string.h>

static short ichans[MAXCHAN];
static short ochans[MAXCHAN];
static long remain[MAXCHAN];
static char *buf[MAXCHAN],*next[MAXCHAN];
static int flags[MAXCHAN];
static char eight[MAXCHAN];
static char valid[MAXCHAN];
#ifndef NO_TERMSIZE
static struct winsize ttysiz[MAXCHAN];
#endif
#ifndef O_NDELAY				/* Posix */
#define O_NDELAY O_NONBLOCK
#endif

static long restored=0;

#ifdef __hpux

#include <sys/types.h>
#include <sys/time.h>
void usleep(usec)
int usec;
{
	struct timeval tv;

	tv.tv_sec = usec / 1000000;
	tv.tv_usec = usec - 1000000 * tv.tv_sec;
	select(ulimit(4,0),NULL,NULL,NULL,&tv);
}
#endif

void fini_string()
{
	int i;
	
	if(restored)
		return;
	for(i=0;i<MAXCHAN;i++)
		if(valid[i])
		{
#ifdef NeXT
			ioctl(ichans[i],TIOCSETP,&savetty[i]);
#else
			tcsetattr(ichans[i],TCSANOW,&savetty[i]);
#endif
			fcntl(ichans[i],F_SETFL,flags[i]);
			close(ichans[i]);
#ifdef NeXT
			ioctl(ochans[i],TIOCSETP,&savetty[i]);
#else
			tcsetattr(ochans[i],TCSANOW,&savetty[i]);
#endif
			fcntl(ochans[i],F_SETFL,flags[i]);
			close(ochans[i]);
		}
	restored=1;
}

int cleanup()
{
	fini_string();
	exit(0);
}

int ttyinit(term,eightbit)
char *term;
int eightbit;
{
	int i,newchan;
#ifdef NeXT
	struct sgttyb newtty;
#else
	struct termios newtty;
#endif

	for(newchan = 0;newchan < MAXCHAN;newchan++)
		if(!valid[newchan])
			break;
	if(newchan == MAXCHAN)
	{
		printf("Too many terminal ports have been assigned.\n");
		cleanup();
	}
	if((ichans[newchan]=open(term,O_RDONLY|O_NDELAY))<0)
		term_error("get channel",term);
	if((flags[newchan] = fcntl(ichans[newchan],F_GETFL,0)) == -1)
		term_error("read flags",term);
	if(fcntl(ichans[newchan],F_SETFL,flags[newchan] | O_NDELAY) == -1)
		term_error("clear blocking",term);
#ifdef NeXT
	if(ioctl(ichans[newchan],TIOCGETP,&savetty[newchan])== -1)
#else
	if(tcgetattr(ichans[newchan],&savetty[newchan]) == -1)
#endif
		term_error("read characteristics",term);
	signal(SIGINT,fini_string);
	newtty=savetty[newchan];
#ifdef NeXT
	newtty.sg_flags |= CBREAK;
	newtty.sg_flags &= ~(CRMOD | ECHO | RAW | TANDEM);
	if(ioctl(ichans[newchan],TIOCSETP,&savetty[newchan])== -1)
#else
	newtty.c_iflag = IGNBRK | IXON | IXOFF;
	newtty.c_oflag = 0;
	newtty.c_cflag = ((eightbit)? CS8 : CS7) | CREAD | CLOCAL;
#ifdef CBAUD
	newtty.c_cflag |= (savetty[newchan].c_cflag & CBAUD);
#endif
	newtty.c_lflag = 0;
	for(i = 0;i < 16;i++)
		newtty.c_cc[i] = 0;
	newtty.c_cc[VMIN]=5;
	newtty.c_cc[VTIME]=1;
	if(tcsetattr(ichans[newchan],TCSANOW,&newtty) == -1)
#endif
		term_error("set raw mode",term);
	remain[newchan]=0;
	if(!(buf[newchan]=(char *)malloc(BUFFER_SIZE*sizeof(char))))
		term_error("get buffer",term);
	if((ochans[newchan]=open(term,O_WRONLY|O_NDELAY))<0)
		term_error("get channel",term);
	if(fcntl(ochans[newchan],F_SETFL,flags[newchan] | O_NDELAY) == -1)
		term_error("clear blocking",term);
#ifdef NeXT
	if(ioctl(ochans[newchan],TIOCSETP,&newtty) == -1)
#else
	if(tcsetattr(ochans[newchan],TCSANOW,&newtty) == -1)
#endif
		term_error("set raw mode",term);
#ifndef NO_TERMSIZE
	/* Don't check for error - we set it anyway and if never set fails */
	ioctl(ochans[newchan],TIOCGWINSZ,&ttysiz[newchan]);
#endif
	remain[newchan]=0;
	if(!(buf[newchan]=(char *)malloc(BUFFER_SIZE*sizeof(char))))
		term_error("get buffer",term);
	eight[newchan] = eightbit;
	valid[newchan] = 1;
	return(newchan);
}

#ifndef NO_TERMSIZE
int ttysetsize(chan,rows,cols)
int chan;
int rows;
int cols;
{
	ttysiz[chan].ws_row = rows;
	ttysiz[chan].ws_col = cols;
	return( ioctl(ochans[chan],TIOCSWINSZ,&ttysiz[chan]) );
}
#endif

int ttyput(chan,c)
int chan;
char *c;
{
	long i;

	if((i = strlen(c)))
		write(ochans[chan],c,i);
	return(i);
}

int tty_timed_raw(chan,c,sec)
int chan;
char *c;
int sec;
{
	int l,ch;
	char *p;
	unsigned long then;

	then = time(NULL);
	while(!remain[chan])
	{
		p=buf[chan];
		ch=ichans[chan];
		while((l=read(ch,p,BUFFER_SIZE-remain[chan]))>0)
		{
			if((remain[chan] += l) >= BUFFER_SIZE)
				break;
			p += l;
		}
		usleep(5000);
		next[chan]=buf[chan];
		if(time(NULL) - then > sec)
			return(0);
	}
	*c = *next[chan]++;
	if(!eight[chan])
		*c &= 0x7f;
	remain[chan]--;
	return(1);
}

void ttyfini(chan)
int chan;
{
#ifdef NeXT
	if(ioctl(ichans[chan],TIOCSETP,&savetty[chan]) == -1)
#else
	if(tcsetattr(ichans[chan],TCSANOW,&savetty[chan]) == -1)
#endif
		term_error("restore cooked mode"," ");
	if(fcntl(ichans[chan],F_SETFL,flags[chan]) == -1)
		term_error("reset blocking"," ");
	if(close(ichans[chan]) < 0)
		term_error("close channel"," ");
#ifdef NeXT
	if(ioctl(ochans[chan],TIOCSETP,&savetty[chan]) == -1)
#else
	if(tcsetattr(ochans[chan],TCSANOW,&savetty[chan]) == -1)
#endif
		term_error("restore cooked mode"," ");
	if(fcntl(ochans[chan],F_SETFL,flags[chan]) == -1)
		term_error("reset blocking"," ");
#ifndef NeXT
	if(close(ochans[chan]) < 0)
		term_error("close channel"," ");
#endif
	free(buf[chan]);
	valid[chan] = 0;
}
#endif

int main(argc,argv)
int argc;
char **argv;
{
	char buf[128],rsp[128],*p;
	int chan,nrow,ncol;
	FILE *fp;
	
#ifdef VMS
	if((chan = ttyinit("tt:",1,0)) < 0)
#else
	if((chan = ttyinit("/dev/tty",1,0)) < 0)
#endif
		exit(0);
	
	sprintf(buf,"%c[2h%c7%c[99;999f%c[6n",27,27,27,27);
	ttyput(chan,buf,strlen(buf));
	tty_timed_raw(chan,rsp,2);
	if(rsp[0] == 27)
	{
		p = rsp;
		while(*p++ != 'R')
			if(!tty_timed_raw(chan,p,1))
				break;
		*p = '\0';
	}
	sprintf(buf,"%c8%c[2l",27,27);
	ttyput(chan,buf,strlen(buf));
	if(sscanf(rsp + 2,"%d;%dR",&nrow,&ncol) == 2)
	{
#ifndef NO_TERMSIZE
		if(ttysetsize(chan,nrow,ncol) == -1)
			printf("Set size failed!\r\n");
#endif
		if(argc == 2 && (fp = fopen("set_size.tmp","w")))
		{
			if(strstr(argv[1],"csh"))
				fprintf(fp,"setenv ROWS %d\nsetenv COLS %d\n",nrow,ncol);
			else
				fprintf(fp,"ROWS=%d;export ROWS\nCOLS=%d;export COLS\n",nrow,ncol);
			fclose(fp);
		}
	}
	ttyfini(chan);
}
